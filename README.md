# HTTP/1.1 Web Server
HTTP/1.1 프로토콜을 준수하는 경량 웹 서버 구현

## 프로젝트 개요
- **개발 기간**: 2024.07 ~ 2024.09
- **사용 기술**: C++, epoll, CGI, Socket Programming

## 주요 기능
- HTTP/1.1 요청 파싱 및 응답 생성
- 정적 파일 서빙 및 디렉토리 리스팅
- CGI를 통한 동적 콘텐츠 처리
- Keep-alive 연결 관리
- Nginx 스타일 설정 파일 파싱

## 핵심 구현 내용
- **논블로킹 I/O**: epoll 기반 이벤트 루프로 동시 연결 처리
- **CGI 프로세스 관리**: fork/exec를 통한 외부 프로그램 실행
- **HTTP 상태 관리**: Keep-alive를 위한 연결 상태 머신 구현

## 기술적 성과
- RFC 2616 표준 준수
- Nginx 설정 파일 형식 호환
- siege 벤치마크 테스트: 255 동시 사용자 처리 확인

## 실행 방법
```bash
make
./webserv [config_file]
```

## 학습 내용
- 네트워크 프로그래밍 및 소켓 API
- I/O 멀티플렉싱 (epoll)
- HTTP 프로토콜 동작 원리
- 프로세스 관리 및 IPC
