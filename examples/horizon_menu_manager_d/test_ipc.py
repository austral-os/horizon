import socket
import json
import sys

def send_request(request):
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        client.connect("/tmp/horizon_menu.sock")
        client.sendall(json.dumps(request).encode('utf-8'))
        
        response = client.recv(4096)
        if response:
            print("Response:", json.loads(response.decode('utf-8')))
        else:
            print("No response received")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    test_request = {
        "type": "create_menu",
        "request_id": "abc123",
        "x": 300,
        "y": 500,
        "menu": {
            "id": "file_menu",
            "title": "File",
            "items": [
                {
                    "id": "new_file",
                    "text": "New",
                    "icon": "document-new",
                    "shortcut": "Ctrl+N"
                },
                {
                    "id": "open_file",
                    "text": "Open",
                    "icon": "document-open",
                    "shortcut": "Ctrl+O"
                },
                {
                    "id": "export_menu",
                    "text": "Export",
                    "submenu": {
                        "id": "export_submenu",
                        "title": "Export As",
                        "items": [
                            {
                                "id": "export_pdf",
                                "text": "PDF"
                            },
                            {
                                "id": "export_png",
                                "text": "PNG"
                            }
                        ]
                    }
                }
            ]
        }
    }
    
    send_request(test_request)
