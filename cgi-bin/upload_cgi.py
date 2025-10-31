#!/usr/bin/env python3

import cgi
import os

print("""
	<html lang="en">
	<head>
	    <meta charset="UTF-8">
	    <meta name="viewport" content="width=device-width, initial-scale=1.0">
	    <title>Document</title>
	</head>
	</html>
""")

upload_dir = os.environ.get('UPLOAD_STORE')

if not upload_dir:
    print("<h1>파일 업로드 불가</h1>")
else:
    if not os.path.exists(upload_dir):
        os.makedirs(upload_dir)

    form = cgi.FieldStorage()
    if "file" in form:
        fileitem = form["file"]
        if fileitem.filename:
            filename = os.path.basename(fileitem.filename)
            filepath = os.path.join(upload_dir, filename)

            with open(filepath, 'wb') as f:
                f.write(fileitem.file.read())

            print(f"<h1>파일 업로드 성공!</h1><p>파일 이름: {filename}</p>")
    else:
        print("<h1>파일을 찾을 수 없습니다.</h1>")

print("""
    <a href="/upload.html"><button>돌아가기</button></a>
</body>
</html>
""")