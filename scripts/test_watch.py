"""Build and execute the production watch rules on the host."""
import os
import subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / '.build' / 'watch-tests'
BUILD.mkdir(parents=True, exist_ok=True)
INCLUDE = ROOT / 'FirmWare/watch/components/dp_motion/include'
VCVARS = Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat'
for name in ('core_tests', 'calibration_tests', 'swipe_tests', 'provisioning_tests', 'recorder_tests', 'time_tests', 'aichat_tests', 'display_tests'):
    executable = BUILD / (name + '.exe')
    source = ROOT / 'tests/watch' / (name + '.cpp')
    command = f'call "{VCVARS}" >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /I"{INCLUDE}" /I"{ROOT / "FirmWare/watch/components/dp_display"}" /I"{ROOT / "FirmWare/watch/components/Recorder"}" /I"{ROOT / "FirmWare/watch/components/dp_time"}" /I"{ROOT / "FirmWare/watch/components/XiaozhiApp"}" /Fe:"{executable}" /Fo:"{BUILD / (name + ".obj")}" "{source}"'
    subprocess.run('cmd /d /s /c "' + command + '"', check=True)
    subprocess.run([str(executable)], check=True)
