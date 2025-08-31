#/usr/bin/env bash
set -e
# '/home/dadrian/.espressif/python_env/idf5.5_py3.13_env/bin/python' '/home/dadrian/esp/v5.5/esp-idf/tools/idf.py' app
'/home/dbadrian/devtools/espressif/python_env/idf5.5_py3.12_env/bin/python' '/home/dbadrian/esp/v5.5/esp-idf/tools/idf.py' app
python remote_update.py