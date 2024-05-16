import os

pwd = os.getcwd()

file = open(pwd + "/build/tutorial/init.conf", "w")

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