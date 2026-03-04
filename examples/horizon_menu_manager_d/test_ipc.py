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
    # A complex professional-grade menu example
    complex_menu = {
        "type": "create_menu",
        "request_id": "complex_test_001",
        "x": 100,
        "y": 100,
        "menu": {
            "id": "main_menu",
            "title": "Horizon Complex Menu",
            "max_width": 300,
            "items": [
                {"id": "new", "text": "New File", "icon": "document-new", "shortcut": "Ctrl+N"},
                {"id": "open", "text": "Open Project...", "icon": "document-open", "shortcut": "Ctrl+O"},
                {"id": "save", "text": "Save Current", "icon": "document-save", "shortcut": "Ctrl+S"},
                {"id": "save_as", "text": "Save As...", "shortcut": "Ctrl+Shift+S"},
                
                # Submenu 1: Edit
                {
                    "id": "edit_menu",
                    "text": "Edit",
                    "submenu": {
                        "id": "sub_edit",
                        "title": "Edit Actions",
                        "items": [
                            {"id": "undo", "text": "Undo", "shortcut": "Ctrl+Z"},
                            {"id": "redo", "text": "Redo", "shortcut": "Ctrl+Y"},
                            # Nested Submenu in Edit
                            {
                                "id": "search_submenu",
                                "text": "Find & Replace",
                                "submenu": {
                                    "id": "sub_search",
                                    "items": [
                                        {"id": "find", "text": "Quick Find", "shortcut": "Ctrl+F"},
                                        {"id": "replace", "text": "Replace", "shortcut": "Ctrl+H"},
                                        {"id": "find_in_files", "text": "Find in Files", "shortcut": "Ctrl+Shift+F"}
                                    ]
                                }
                            },
                            {"id": "cut", "text": "Cut", "shortcut": "Ctrl+X"},
                            {"id": "copy", "text": "Copy", "shortcut": "Ctrl+C"},
                            {"id": "paste", "text": "Paste", "shortcut": "Ctrl+V"}
                        ]
                    }
                },
                
                # Submenu 2: Layout/View
                {
                    "id": "view_menu",
                    "text": "View",
                    "icon": "view-node",
                    "submenu": {
                        "id": "sub_view",
                        "items": [
                            {"id": "zoom_in", "text": "Zoom In", "shortcut": "Ctrl++"},
                            {"id": "zoom_out", "text": "Zoom Out", "shortcut": "Ctrl+-"},
                            # Nested Submenu in View
                            {
                                "id": "appearance_submenu",
                                "text": "Appearance",
                                "submenu": {
                                    "id": "sub_appr",
                                    "max_width": 180,
                                    "items": [
                                        {"id": "full_screen", "text": "Toggle Full Screen", "shortcut": "F11"},
                                        {"id": "zen_mode", "text": "Zen Mode", "shortcut": "Ctrl+K Z"},
                                        {"id": "centered", "text": "Centered Layout"}
                                    ]
                                }
                            }
                        ]
                    }
                },
                
                # Submenu 3: Development Tools
                {
                    "id": "tools_menu",
                    "text": "Tools",
                    "submenu": {
                        "id": "sub_tools",
                        "items": [
                            {"id": "build", "text": "Build Solution", "shortcut": "F7"},
                            {"id": "debug", "text": "Run Debugger", "icon": "system-run", "shortcut": "F5"},
                            # Nested Submenu in Tools
                            {
                                "id": "git_submenu",
                                "text": "Source Control (Git)",
                                "submenu": {
                                    "id": "sub_git",
                                    "items": [
                                        {"id": "git_commit", "text": "Commit...", "shortcut": "Ctrl+Enter"},
                                        {"id": "git_push", "text": "Push Changes"},
                                        {"id": "git_pull", "text": "Pull from Origin"}
                                    ]
                                }
                            }
                        ]
                    }
                },
                
                # Submenu 4: Help
                {
                    "id": "help_menu",
                    "text": "Help",
                    "icon": "help-about",
                    "submenu": {
                        "id": "sub_help",
                        "items": [
                            {"id": "welcome", "text": "Welcome Screen"},
                            {"id": "docs", "text": "Documentation"},
                            {"id": "about", "text": "About Horizon"}
                        ]
                    }
                },
                
                {"id": "settings", "text": "Preferences...", "icon": "preferences-system"},
                {"id": "exit", "text": "Exit Application", "shortcut": "Ctrl+Q"}
            ]
        }
    }
    
    send_request(complex_menu)
