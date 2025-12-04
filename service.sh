#!/bin/bash

# --- 설정 변수 ---
PORT=5555
NODE_APP="servers/server.js" # Node.js 앱의 실제 경로 및 파일명
LOG_FILE="server.log"
# ------------------

case "$1" in
    start)
        echo "Starting Node.js server on port $PORT..."
        # nohup을 사용하여 백그라운드에서 실행하고 로그 파일로 출력 리디렉션
        nohup node $NODE_APP $PORT > $LOG_FILE 2>&1 &
        echo "Server started. Check $LOG_FILE for logs."
        ;;
    stop)
        echo "Stopping Node.js server on port $PORT..."
        # 해당 포트를 사용하는 프로세스 ID (PID) 찾기
        PID=$(lsof -t -i :$PORT)

        if [ -z "$PID" ]; then
            echo "Warning: No server found running on port $PORT."
        else
            kill -9 $PID # 강제 종료
            echo "Server (PID: $PID) stopped."
        fi
        ;;
    status)
        PID=$(lsof -t -i :$PORT)
        if [ -z "$PID" ]; then
            echo "Server is not running on port $PORT."
        else
            echo "Server is running (PID: $PID) on port $PORT."
        fi
        ;;
    *)
        echo "Usage: $0 {start|stop|status}"
        exit 1
        ;;
esac

exit 0