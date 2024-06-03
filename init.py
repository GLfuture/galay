import os
import json

pwd = os.getcwd()

file = open(pwd + "/build/test/init.conf", "w")

file.write(
    "author=gong\n"
    "title=tutorial\n"
    "description=tutorial\n"
    "version=1.0.0\n"
    "date=2020-01-01\n"
    "license=MIT\n"
    "copyright=gong\n"
    "email=gong@gmail.com\n"
    "website=https://github.com/GLfuture/galay\n"
    "keywords=tutorial\n"
)

file.close()

file = open(pwd + "/build/test/init.json", "w")
data = {
    "author": "gong",
    "age": 21,
    "is_student": True,
    "title": "tutorial",
    "description": "tutorial",
    "version": "1.0.0",
    "date": "2024-06-02",
    "license": "MIT",
    "copyright": "gong",
    "email": "gong@gmail.com",
    "website": "https://github.com/GLfuture/galay",
    "keywords": "tutorial",
    "dependencies": ["nlohmann/json","openssl"]
}
file.write(json.dumps(data))
file.close()