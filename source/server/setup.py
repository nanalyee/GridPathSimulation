from cx_Freeze import setup, Executable

# Dependencies are automatically detected, but some might need fine tuning


# with open('requ.txt', 'r', encoding='utf16') as f:
#     packages = [line.strip().split('==')[0] for line in f]

build_exe_options = {
    "packages": ["os"], 
}

setup(
    name="GridPathServer",
    version="0.1",
    description="Grid Path Server App",
    options={"build_exe": build_exe_options},
    executables=[Executable("server.py")]
)