#!/usr/bin/env python3

import cgi

print("""
	<html lang="en">
	<head>
	    <meta charset="UTF-8">
	    <meta name="viewport" content="width=device-width, initial-scale=1.0">
	    <title>Document</title>
	</head>
	</html>
""")

form = cgi.FieldStorage()

try:
    num1 = int(form.getvalue("num1"))
    op = form.getvalue("op")
    num2 = int(form.getvalue("num2"))
    if (op == '+'):
      result = num1 + num2
    elif (op == '-'):
      result = num1 - num2
    elif (op == '*'):
      result = num1 * num2
    elif (op == '/'):
      result = num1 / num2
    else:
      raise TypeError

    print(f"""
    <html>
      <body>
        <h1>결과: {result}</h1>
        <a href="/index.html"><button>돌아가기</button></a>
      </body>
    </html>
    """)
except (TypeError, ValueError, ZeroDivisionError):
    print("""
    <html>
      <body>
        <h1>오류: 숫자를 제대로 입력하세요.</h1>
        <a href="/index.html"><button>돌아가기</button></a>
      </body>
    </html>
    """)
