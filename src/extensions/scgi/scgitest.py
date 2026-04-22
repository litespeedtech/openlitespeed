#!/usr/bin/python3
import codecs, logging, os, pwd, socket, subprocess, sys, time
from scgi.util import SCGIHandler
from scgi.scgi_server import SCGIServer
from scgi.systemd_socket import get_systemd_socket

#Use: You must configure LiteSpeed for SCGI.  If you modify $LSWS_HOME/conf/httpd_config.conf
#extprocessor scgiapp {
#  type                    scgi
#  address                 localhost:4000
#  maxConns                20
#  initTimeout             60
#  retryTimeout            0
#  respBuffer              0
#  autoStart               0
#}
# ...and in scripthandler:
#  add                     scgi:scgiapp scgi
# Create a simple test trigger program in Example/html like test.scgi
# Then start this program.  Contact it in your browser with :
# http://127.0.0.1/test.scgi
# It will output all of the environment variables and the contents of the script.
#
# Note I also tested SCGI with Fossil and it seems ok.
# In my fossil directory:
#   cd trunk
#   ../fossil server --scgi --port 4000
#   In the browser http://127.0.0.1/test.scgi
#
# To test a large file: curl -X POST -H "Content-Type: text/plain" --data @/usr/local/lsws/Example/html/10mb.html http://127.0.0.1/httpsession.scgi
# To test chunking: curl -X POST -H "Content-Type: text/plain" -H "Transfer-Encoding: chunked" --data @/usr/local/lsws/Example/html/10mb.html http://127.0.0.1/httpsession.scgi
# Can now be used with uds="<socket file>" in MyAppServer instread of host="IP Address"
# Can not be successfully started by LiteSpeed, must be manually started.

def ns_read_size(input):
    size = b''
    while 1:
        c = input.read(1)
        if c == b':':
            break
        elif not c:
            raise IOError('short netstring read')
        size = size + c
    return int(size)


def ns_reads(input):
    size = ns_read_size(input)
    data = b''
    while size > 0:
        s = input.read(size)
        if not s:
            raise IOError('short netstring read')
        data = data + s
        size -= len(s)
    if input.read(1) != b',':
        raise IOError('missing netstring terminator')
    return data


# string encoding for evironmental variables
HEADER_ENCODING = 'iso-8859-1'


def parse_env(headers):
    items = headers.split(b'\0')
    items = items[:-1]
    if len(items) % 2 != 0:
        raise ValueError('malformed headers')
    env = {}
    for i in range(0, len(items), 2):
        k = items[i].decode(HEADER_ENCODING)
        v = items[i+1].decode(HEADER_ENCODING)
        env[k] = v
    return env


def read_env(input):
    headers = ns_reads(input)
    return parse_env(headers)


class MyAppHandler(SCGIHandler):
    def conn_output(self, env, bodysize, input, conn):
        logging.debug("Entering MyAppHandler.conn_output")
        blocks = 0
        blocksize = 16384
        inputBytes = 0
        if bodysize != 0:
            try:
                remaining = bodysize
                logging.debug("Read body size: %d" % bodysize)
                while remaining > 0:
                    if remaining > blocksize:
                        readsize = blocksize
                    else:
                        readsize = remaining
                    block = input.read(readsize)
                    if not block:
                        logging.debug("Read no data (unexpected)")
                        break
                    blocks += 1
                    readlen = len(block)
                    remaining -= readlen
                    inputBytes += readlen
            except BrokenPipeError:
                logging.debug("BrokenPipeError occurred received %d blocks %d bytes" % (blocks, inputBytes))
                return
            except Exception as e:
                logging.debug("Error occurred in receive %d blocks %d bytes: %s" % (blocks, inputBytes, str(e)))
                return
            
        logging.debug("Read %d blocks %d bytes" % (blocks, inputBytes))
        bytes = 0
        linenumber = 0
        strPrint = "Content-Type: text/plain; charset=utf-8\n"
        conn.sendall(strPrint.encode('utf-8'))
        bytes += len(strPrint)
        linenumber += 1
        strPrint = '\n'
        conn.sendall(strPrint.encode('utf-8'))
        bytes += len(strPrint)
        linenumber += 1
        strPrint = 'SCGI test program.  Environment variables:\n'
        conn.sendall(strPrint.encode('utf-8'))
        bytes += len(strPrint)
        linenumber += 1
        script_filename = ''
        sleep_time = 0
        for k, v in env.items():
            logging.debug("Env key: %s, value: %r" % (k, v))
            strPrint = "%s: %r\n" % (k, v)
            conn.sendall(strPrint.encode('utf-8'))
            bytes += len(strPrint)
            linenumber += 1
            if k == 'SCRIPT_FILENAME':
                script_filename = v.replace("'", "")
            elif k == "QUERY_STRING" and v != "":
                sleep_time = int(v.replace("'", ""))
                if sleep_time != 0:
                    logging.debug("Sleep for %d seconds" % sleep_time)
                    time.sleep(sleep_time)
        logging.debug("Finished in child")
        if script_filename != '':
            strPrint = 'End of Environment variables, begin %s\n' % script_filename
            conn.sendall(strPrint.encode('utf-8'))
            bytes += len(strPrint)
            linenumber += 1
            logging.debug("Script file: %s" % script_filename)
            error = False
            with open(script_filename, 'r') as f:
                try:
                    for line in f:
                        try:
                            strPrint = line
                            #logging.debug("line: %s" % line)
                            conn.sendall(strPrint.encode('utf-8'))
                            bytes += len(strPrint)
                            linenumber += 1
                            if linenumber % 1000000 == 0:
                                logging.debug("Sent line #%d" % linenumber)
                        except BrokenPipeError:
                            logging.debug("BrokenPipeError occurred sent %d lines %d bytes" % (linenumber, bytes))
                            error = True
                            break
                        except:
                            logging.debug("Error occurred in print sent %d lines %d bytes" % (linenumber, bytes))
                            error = True
                            break
                except:
                    logging.debug("Error occurred sent %d lines %d bytes" % (linenumber, bytes))
                    error = True
            if not error:
                logging.debug("Sent complete file, %d lines %d bytes" % (linenumber, bytes))

    def handle_connection(self, conn):
        logging.debug("handle_connection")
        input = conn.makefile("rb")
        env = self.read_env(input)
        bodysize = int(env.get('CONTENT_LENGTH', 0))
        logging.debug("handle_connection, read env, length: %d" % bodysize)
        #stdin = sys.stdin
        #sys.stdin = codecs.getreader('utf-8')(input)
        try:
            self.conn_output(env, bodysize, input, conn)
        finally:
            #sys.stdin.close()
            #sys.stdin = stdin
            input.close()

    def produce_cgilike(self, env, bodysize):
        logging.debug("Entering MyAppHandler.produce_cgilike")
        for line in sys.stdin:
            logging.debug("stdin line length: {}".format(len(line)))
        bytes = 80
        strPrint = "Content-Type: text/plain; charset=utf-8"
        print(strPrint)
        bytes += (len(strPrint) + 2)
        print()
        bytes += 2
        strPrint = 'SCGI test program.  Environment variables:'
        print(strPrint)
        bytes += (len(strPrint) + 2)
        script_filename = ''
        sleep_time = 0
        for k, v in env.items():
            logging.debug("Env key: %s, value: %r" % (k, v))
            strPrint = "%s: %r" % (k, v)
            print(strPrint)
            bytes += (len(strPrint) + 2)
            if k == 'SCRIPT_FILENAME':
                script_filename = v.replace("'", "")
            elif k == "QUERY_STRING" and v != "":
                sleep_time = int(v.replace("'", ""))
                if sleep_time != 0:
                    logging.debug("Sleep for %d seconds" % sleep_time)
                    time.sleep(sleep_time)
        logging.debug("Finished in child")
        if script_filename != '':
            strPrint = 'End of Environment variables, begin %s' % script_filename
            print(strPrint)
            bytes += (len(strPrint) + 2)
            logging.debug("Script file: %s" % script_filename)
            linenumber = 0
            error = False
            with open(script_filename, 'r') as f:
                try:
                    for line in f:
                        try:
                            strPrint = line
                            #logging.debug("line: %s" % line)
                            print(strPrint, end='')
                            bytes += (len(strPrint) + 1)
                            linenumber += 1
                            if linenumber % 1000000 == 0:
                                logging.debug("Sent line #%d" % linenumber)
                        except BrokenPipeError:
                            logging.debug("BrokenPipeError occurred sent %d lines %d bytes" % (linenumber, bytes))
                            error = True
                            break
                        except:
                            logging.debug("Error occurred in print sent %d lines %d bytes" % (linenumber, bytes))
                            error = True
                            break
                except:
                    logging.debug("Error occurred sent %d lines %d bytes" % (linenumber, bytes))
                    error = True
            if not error:
                logging.debug("Sent complete file, %d lines %d bytes" % (linenumber, bytes))


class MyAppServer(SCGIServer):
    DEFAULT_PORT = 4000

    def __init__(self, handler_class=SCGIHandler, host="", port=DEFAULT_PORT, max_children=5, uds=""):
        super().__init__(handler_class, host, port, max_children)
        self.uds = uds

    def get_listening_socket(self):
        if self.uds != "":
            logging.debug('Using uds: %s' % self.uds)
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            if os.path.exists(self.uds):
                os.remove(self.uds)
            os.umask(0)
            s.bind(self.uds)
        else:
            logging.debug('Using addr %s: port %d' % (self.host, self.port))
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind((self.host, self.port))
        return s

def main():
    print('SCGI test program.  See comments in this program for use')
    print("Use a query string to specify number of seconds to sleep")
    os.umask(0)
    logging.basicConfig(filename='/tmp/scgi.log', 
                        format="%(asctime)s.%(msecs)03d" + " [%(levelname)s] %(message)s",
                        level=logging.DEBUG,
                        datefmt="%Y-%m-%d %H:%M:%S")
    logging.debug("Entering scgitest")
    try:
        MyAppServer(handler_class=MyAppHandler, host="127.0.0.1").serve()
    except IOError as e:
        logging.debug("IOError Exception %s" % str(e))
    except Exception as e:
        logging.debug("Exception %s" % str(e))
    logging.debug("Exiting scgitest")

if __name__ == "__main__":
    main()
