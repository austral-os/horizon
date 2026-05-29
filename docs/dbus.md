# Using D-Bus in Horizon OS

This document details the process of creating and consuming D-Bus services within the Horizon framework. It uses existing implementations from projects like `horizon-lens` and `horizon-keyring` as references.

Horizon provides a C++ wrapper called `dbusutils::DbusHelper` located at `horizon/dbusutils/DbusHelper.hpp`. This class simplifies the interaction with `libdbus-1`, making it easier to expose objects (Server/Service) and invoke remote methods (Client).

---

## 1. Core Concepts

- **`DbusHelper`**: The main class that manages the connection to a bus (`DBUS_BUS_SESSION` or `DBUS_BUS_SYSTEM`) and provides methods to register objects, request names, perform calls, and listen for events.
- **`DbusObject`**: The base class you must inherit from to create a service. It requires you to implement the `handle_message()` method.
- **`DbusVariant`**: An alias for `std::variant` that represents common D-Bus data types (`int32_t`, `std::string`, `std::vector`, `a{sv}` dictionaries, etc.), simplifying the handling of complex arguments and responses.

---

## 2. Creating a D-Bus Service (Server)

To expose a D-Bus API (for example, the thumbnail generator in `horizon-lens` or the secrets manager in `horizon-keyring`), you should follow these steps:

### 2.1 Create the Service Class

You must inherit from `horizon::dbusutils::DbusObject` and override the pure virtual method `handle_message`.

**Example (`MyService.hpp`):**
```cpp
#pragma once

#include <horizon/dbusutils/DbusHelper.hpp>

class MyService : public horizon::dbusutils::DbusObject
{
public:
    explicit MyService(horizon::dbusutils::DbusHelper& dbus);
    ~MyService() override = default;

    DBusHandlerResult handle_message(DBusConnection* conn, DBusMessage* msg) override;

private:
    horizon::dbusutils::DbusHelper& m_dbus;
    
    void handle_my_method(DBusMessage* msg);
};
```

### 2.2 Implement Message Processing

The `handle_message` method receives every incoming message. Here, you should filter by **interface** and **method**. It is also good practice to handle `Introspect` (interface `org.freedesktop.DBus.Introspectable`) by returning the XML that defines your API, as this helps tools like `d-feet` or dynamic clients.

**Example (`MyService.cpp`):**
```cpp
#include "MyService.hpp"
#include <horizon/Logger.hpp>

MyService::MyService(horizon::dbusutils::DbusHelper& dbus) : m_dbus(dbus) {}

DBusHandlerResult MyService::handle_message(DBusConnection* conn, DBusMessage* msg)
{
    std::string interface = dbus_message_get_interface(msg) ? dbus_message_get_interface(msg) : "";
    std::string method = dbus_message_get_member(msg) ? dbus_message_get_member(msg) : "";

    // 1. Handle our interface's methods
    if (interface == "org.horizon.MyInterface") {
        if (method == "DoSomething") {
            handle_my_method(msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }
    
    // 2. Optional support for Introspection
    if (interface == "org.freedesktop.DBus.Introspectable" && method == "Introspect") {
        std::string xml = R"(<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.horizon.MyInterface">
    <method name="DoSomething">
      <arg name="parameter" type="s" direction="in"/>
    </method>
  </interface>
</node>)";
        m_dbus.send_reply(msg, {xml});
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    // If we don't recognize the message, pass control.
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
```

### 2.3 Process Arguments and Send Replies

If the method receives arguments, you can extract them using native libdbus functions (like `dbus_message_get_args`) and then respond using `m_dbus.send_reply`.

```cpp
void MyService::handle_my_method(DBusMessage* msg)
{
    const char* param_str = nullptr;
    
    // Read a String ('s') parameter
    if (!dbus_message_get_args(msg, nullptr, DBUS_TYPE_STRING, &param_str, DBUS_TYPE_INVALID)) {
        LOG_ERROR << "Invalid arguments.";
        m_dbus.send_error(msg, "org.horizon.Error.InvalidArgs", "Expected a string.");
        return;
    }

    LOG_INFO << "DoSomething called with: " << param_str;

    // Process logic here...
    
    // Send a reply. If the method returns void, send an empty response.
    m_dbus.send_reply(msg, {}); 
}
```

### 2.4 Register and Start the Service in Main

For the service to exist on the message bus system:

```cpp
#include <horizon/dbusutils/DbusHelper.hpp>
#include "MyService.hpp"

int main(int argc, char** argv) {
    // 1. Create the Helper connecting to the session bus (DBUS_BUS_SESSION)
    horizon::dbusutils::DbusHelper dbus(DBUS_BUS_SESSION);
    
    // 2. Request a well-known name on the bus
    if (!dbus.request_name("org.horizon.MyService")) {
        return 1; // Error or service already running
    }
    
    // 3. Instantiate the service
    MyService service(dbus);
    
    // 4. Expose it at a specific Object Path
    dbus.register_object("/org/horizon/MyService", &service);
    
    // 5. Enter an infinite event processing loop
    while (true) {
        dbus.process_events(100);
        // ... other asynchronous tasks ...
    }
    
    return 0;
}
```

---

## 3. Consuming a D-Bus Service (Client)

To make requests from a client, `DbusHelper` offers several methods that assist with calling and parsing.

### 3.1 "Fire and Forget" Requests (Asynchronous / No Reply Expected)

Just like it's done in `ThumbnailCache::request_thumbnail` to request thumbnail generation from `horizon-lens`:

```cpp
#include <horizon/dbusutils/DbusHelper.hpp>
#include <dbus/dbus.h>

void request_action(const std::string& arg)
{
    horizon::dbusutils::DbusHelper dbus(DBUS_BUS_SESSION);
    
    // 1. Create the method call message
    DBusMessage* msg = dbus_message_new_method_call(
        "org.horizon.MyService",      // Destination name
        "/org/horizon/MyService",     // Object Path
        "org.horizon.MyInterface",    // Interface
        "DoSomething"                 // Method
    );
    
    if (msg) {
        // 2. Append arguments using native libdbus
        const char* path_cstr = arg.c_str();
        dbus_message_append_args(msg, DBUS_TYPE_STRING, &path_cstr, DBUS_TYPE_INVALID);
        
        // 3. Send message asynchronously ("fire and forget")
        dbus_connection_send(dbus.get_connection(), msg, nullptr);
        dbus_connection_flush(dbus.get_connection());
        
        // 4. Free memory
        dbus_message_unref(msg);
    }
}
```

### 3.2 Synchronous Requests

To wait for results, `DbusHelper` provides `call_method_sync` (if passing arguments via `DbusVariant`) or `call_method` for simple requests with no parameters or pre-formatted ones.

```cpp
#include <horizon/dbusutils/DbusHelper.hpp>
#include <vector>

void consume_service_sync()
{
    horizon::dbusutils::DbusHelper dbus(DBUS_BUS_SESSION);

    // Prepare arguments using the Variant wrapper
    std::vector<horizon::dbusutils::DbusVariant> args = {
        std::string("parameter 1"),
        (uint32_t)100
    };

    // Make the synchronous call
    DBusMessage* reply = dbus.call_method_sync(
        "org.horizon.Example",
        "/org/horizon/Example",
        "org.horizon.Example.Interface",
        "Method",
        args,
        1000 // timeout in milliseconds
    );

    if (reply) {
        // Process the reply
        DBusError error;
        dbus_error_init(&error);
        
        const char* response = nullptr;
        if (dbus_message_get_args(reply, &error, DBUS_TYPE_STRING, &response, DBUS_TYPE_INVALID)) {
            // Successfully got the response
        }
        
        // You must unref the reply message
        dbus_message_unref(reply);
    }
}
```

### 3.3 Reading Properties (`GetProperty`)

If the service interface implements the standard Properties interface (e.g., interfaces in NetworkManager or Disks), `DbusHelper` includes a helper method (`get_property`):

```cpp
horizon::dbusutils::DbusHelper dbus(DBUS_BUS_SYSTEM);
horizon::dbusutils::DbusVariant prop_val = dbus.get_property(
    "org.freedesktop.UDisks2", 
    "/org/freedesktop/UDisks2/block_devices/sda",
    "org.freedesktop.UDisks2.Block",
    "IdLabel"
);

// Handle the returned value depending on its underlying variant type
if (std::holds_alternative<std::string>(prop_val)) {
    std::string label = std::get<std::string>(prop_val);
}
```

---

## Conclusion

The `dbusutils::DbusHelper` component serves as the foundation for both exporting powerful system APIs and consuming them cleanly from applications like `arkfm`, `top_panel`, or viewers. The main recommendation is to structure each service as its own `DbusObject` class, implement `Introspectable` so it is easy to debug with external tools, and always consider whether the client response should be a synchronous block or an asynchronous `fire and forget` to maintain responsive interfaces.
