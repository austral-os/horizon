import os
import re

libs_dir = "/home/horacio/Desarrollo/horizon/libs"
for lib in os.listdir(libs_dir):
    lib_path = os.path.join(libs_dir, lib)
    if os.path.isdir(lib_path):
        cmake_file = os.path.join(lib_path, "CMakeLists.txt")
        if os.path.exists(cmake_file):
            with open(cmake_file, "r") as f:
                content = f.read()

            # Replace 'COMPONENT core' with 'COMPONENT lib${lib}'
            # Also replace any other 'COMPONENT something' with 'COMPONENT lib${lib}' for library targets if we want
            # but let's just specifically look for 'install(TARGETS libname DESTINATION lib COMPONENT ...)'
            
            # Use regex to find install(TARGETS <lib> ... COMPONENT core)
            content = re.sub(
                r'(install\s*\(\s*TARGETS\s+'+lib+r'\s+DESTINATION\s+lib\s+COMPONENT\s+)core(\s*\))',
                r'\g<1>lib'+lib+r'\g<2>',
                content
            )

            # Check if include directory exists
            include_dir = os.path.join(lib_path, "include")
            if os.path.exists(include_dir) and "install(DIRECTORY include/" not in content:
                # Add install(DIRECTORY include/ DESTINATION include COMPONENT lib${lib}-dev)
                install_inc = f"\ninstall(DIRECTORY include/\n    DESTINATION include\n    COMPONENT lib{lib}-dev\n)\n"
                content += install_inc

            with open(cmake_file, "w") as f:
                f.write(content)
