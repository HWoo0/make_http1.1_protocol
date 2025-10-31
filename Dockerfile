FROM ubuntu:20.04

# 업데이트 / siege 설치
RUN apt-get update && apt-get install -y siege

# 디렉토리 설정
WORKDIR /siege

# PATH 추가
ENV PATH="/usr/local/bin:$PATH"
