PORT=$(((RANDOM % 10) + 8080))

make clean
make

while [ `lsof -i :$PORT | wc -l` -gt 0 ]; do
    PORT=$(((RANDOM % 10) + 8080))
done

./httpserver $PORT -l server.log -t 3 &> /dev/null &

if [ `ps -ef | grep "server $PORT" | grep -v grep | wc -l` -gt 0 ]; then
    echo -e "\033[32mServer is already running!\033[0m\n"
else
    echo -e "\033[31mServer is not running!\033[0m\n"
    exit 1
fi

echo -e "\033[35m**Test for GET (404)**\033[0m"                   # Get Test start!

CODE=`curl -s -o /dev/null -w "%{http_code}" http://localhost:$PORT`
if [ $CODE -eq 404 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

echo -e "\033[35m**Test for GET (200)**\033[0m"
echo "Hello World!" > text.txt
CODE=`curl -s -o /dev/null -w "%{http_code}" http://localhost:$PORT/text.txt`
if [ $CODE -eq 200 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

echo -e "\033[35m**Test for GET (403)**\033[0m"
chmod 200 text.txt
CODE=`curl -s -o /dev/null -w "%{http_code}" http://localhost:$PORT/text.txt`
if [ $CODE -eq 403 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

rm text.txt

echo -e "\033[35m**Test for PUT (201)**\033[0m"
rm -f foo.txt

CODE=`printf "PUT /foo.txt HTTP/1.1\r\nContent-Length: 12\r\n\r\nHello World!" | nc localhost $PORT | grep "HTTP/1.1" | awk '{print $2}'`
if [ $CODE -eq 201 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

echo -e "\033[35m**Test for PUT (200)**\033[0m"

CODE=`printf "PUT /foo.txt HTTP/1.1\r\nContent-Length: 12\r\n\r\nHello World!" | nc localhost $PORT | grep "HTTP/1.1" | awk '{print $2}'`
if [ $CODE -eq 200 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

echo -e "\033[35m**Test for PUT (403)**\033[0m"
chmod 400 foo.txt
CODE=`printf "PUT /foo.txt HTTP/1.1\r\nContent-Length: 12\r\n\r\nHello World!" | nc localhost $PORT | grep "HTTP/1.1" | awk '{print $2}'`

if [ $CODE -eq 403 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

rm -f foo.txt

echo -e "\033[35m**Test for HEAD (404)**\033[0m"

# Let code equals to status code of HEAD request
CODE=`printf "HEAD / HTTP/1.1\r\n\r\n" | nc localhost $PORT | grep "HTTP/1.1" | awk '{print $2}'`
if [ $CODE -eq 404 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

echo -e "\033[35m**Test for HEAD (200)**\033[0m"
echo "Hello World!" > text.txt
CODE=`printf "HEAD /text.txt HTTP/1.1\r\n\r\n" | nc localhost $PORT | grep "HTTP/1.1" | awk '{print $2}'`
if [ $CODE -eq 200 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

echo -e "\033[35m**Test for HEAD (403)**\033[0m"
chmod 200 text.txt
CODE=`printf "HEAD /text.txt HTTP/1.1\r\n\r\n" | nc localhost $PORT | grep "HTTP/1.1" | awk '{print $2}'`
if [ $CODE -eq 403 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

rm text.txt

echo -e "\033[35m**Test for GET (200) 100 times**\033[0m"

START=$(date +%s.%N)
for i in {1..100}
do
    curl -s -H "Request-Id: $i" http://localhost:$PORT/queue.c > /dev/null
done
END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)
if [ $? -eq 0 ]; then
    echo -e "\033[32mPassed Time: $DIFF s\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

echo -e "\033[35m**Test for Audit Log**\033[0m"
DIFF=$(diff server.log samplelog.txt)
if [ $? -eq 0 ]; then
    echo -e "\033[32mPassed!\033[0m\n"
else
    echo -e "\033[31mFailed...\033[0m\n"
    exit 1
fi

killall -s SIGTERM httpserver
echo "Wait 5 sec whatever the server is running or not...\n"
sleep 5
killall -s SIGKILL httpserver
