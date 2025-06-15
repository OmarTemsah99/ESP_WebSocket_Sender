from pathlib import Path
import shutil
import os
Import("env")

# Ensure the data directory exists
data_dir = os.path.join(env.get("PROJECT_DIR"), "data")
if not os.path.exists(data_dir):
    os.makedirs(data_dir)

# Print the content of the data directory for debugging
print("Contents of data directory:")
for file in os.listdir(data_dir):
    print(f"- {file}")


def before_buildfs(source, target, env):
    print("Preparing filesystem...")


before_buildfs(None, None, env)
env.AddPreAction("$BUILD_DIR/spiffs.bin", before_buildfs)
