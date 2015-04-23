import tornado.ioloop
import tornado.web
import tornado.options
import requests

import traceback
import xdrlib

domain = "10.51.50.111"

class AuthProxy(tornado.web.RequestHandler):

    def get(self, device):
        try:
            params = {'version': "1", 'key': self.get_argument('key')}
            url = "https://sensorcloud.microstrain.com/SensorCloud/devices/%s/authenticate/" % device
            headers = {'user-agent': "micro-sensorcloud-proxy", 'accept': "<application/xdr>"}
            r = requests.get(url, params=params, headers=headers)

            if r.status_code == requests.codes.ok:
                unpacker = xdrlib.Unpacker(r.content)
                token = unpacker.unpack_string()

                packer = xdrlib.Packer()
                packer.pack_string(token)
                packer.pack_string(domain)
                packer.pack_string("")
                data = packer.get_buffer()
                
                print "parse success"
                self.write(data)
            else:
                print "invalid"
                self.send_error(r.status_code)
        except:
            traceback.print_exc()
            self.send_error(401)

if __name__ == "__main__":
    tornado.options.parse_command_line()
    application = tornado.web.Application([(r"/SensorCloud/devices/([^/]+)/authenticate/", AuthProxy)])
    application.listen(8000)
    tornado.ioloop.IOLoop.instance().start()
