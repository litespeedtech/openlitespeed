#!/bin/python3

from wsgiref.simple_server import make_server
import html

#Use: You must configure LiteSpeed for uWSGI.  If you modify $LSWS_HOME/conf/httpd_config.conf or your vhost.conf file:
#extprocessor uwsgiapp {
#  type                    uwsgi
#  address                 localhost:3031
#  maxConns                20
#  initTimeout             60
#  retryTimeout            0
#  persistConn             1
#  respBuffer              0
#  autoStart               0
#  instances               1
#}
# ...and in scripthandler:
#  add                     uwsgi:uwsgiapp uwsgi
# Install uwsgi on your system.  You must also install uwsgi-plugin-python3
# The easiest way to run it is by setting symbolic links or copy uwsgitest.ini and uwsgitest.py to your Example/html directory and running:
#    uwsgi uwsgitest.ini
#    
# Create a simple test trigger program in Example/html like test.uwsgi
# Then start this program.  Contact it in your browser with :
# http://127.0.0.1/test.uwsgi
# It will output all of the environment variables and the contents of the script as well as any posted input
#
# To test a large file: curl -X POST -H "Content-Type: text/plain" --data @/usr/local/lsws/Example/html/10mb.html http://127.0.0.1/httpsession.uwsgi
# To test chunking: curl -X POST -H "Content-Type: text/plain" -H "Transfer-Encoding: chunked" --data @/usr/local/lsws/Example/html/10mb.html http://127.0.0.1/httpsession.uwsgi
# Can not be successfully started by LiteSpeed, must be manually started.

def application(environ, start_response):
    start_response("200 OK", [('Content-Type', 'text/html')])
    resp = "<p>Environment:</p>"
    resp += "<ul>"
    script_filename = ""
    content_length = 0
    for key, value in environ.items():
        resp += "  <li>" + key + " = " + str(value) + "</li>"
        if key == 'SCRIPT_FILENAME':
            script_filename = str(value).replace("'", "")
        elif key == "CONTENT_LENGTH":
            content_length = int(str(value))
    resp += "</ul>"
    if content_length != 0:
        resp += "Begin POST data:<br>"
        resp += "<pre>"
        resp += environ['wsgi.input'].read(content_length).decode('utf-8')
        resp += "</pre>"
    if script_filename != '':
        resp += 'Begin %s:<br>' % script_filename
        resp += '<pre>'
        with open(script_filename, 'r') as f:
            for line in f:
                resp += html.escape(line)
        resp += '</pre>'

    return resp.encode("utf-8")

