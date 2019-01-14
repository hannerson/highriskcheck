#-*- coding:utf-8 -*-

from pooldb import *
import hashlib
import traceback
import urllib2
import urllib
import json
import time
import requests

sys.setdefaultencoding("utf-8")
reload(sys)

def checkValid(timestamp,digest):
	try:
		if g_config_parser.has_option('common','private_key'):
			private_key = g_config_parser.get('common','private_key')
		else:
			return False
		hl = hashlib.md5()
		hl.update(str("%s_%s" % (private_key,timestamp)).encode(encoding='utf-8'))
		if digest == h1.hexdigest():
			return True
		else:
			return False
	except Exception,e:
		traceback.print_exc()

def client(url):
	try:
		if g_config_parser.has_option('common','private_key'):
			private_key = g_config_parser.get('common','private_key')
		else:
			return False
		hl = hashlib.md5()
		t = int(time.time())
		hl.update(str("%s_%s" % (private_key,t)).encode(encoding='utf-8'))
		digest = hl.hexdigest()
		
		music = "长安"
		artist = "蒋明"
		album = "fdfd"
		post_dict = {}
		post_dict["timestamp"] = "%s" % t
		post_dict["pkey"] = "%s" % digest
		post_dict["music"] = "%s" % music
		post_dict["album"] = "%s" % album
		post_dict["artist"] = "%s" % artist

		#post_data = json.dumps(post_dict)
		params = urllib.urlencode(post_dict)
		#print post_data
		print url + "?" + params
		f = urllib2.urlopen(
			url = url + "?" + params 
		)
		result = f.read()
		print result
	except Exception,e:
		traceback.print_exc()

def client2(url):
	try:
		if g_config_parser.has_option('common','private_key'):
			private_key = g_config_parser.get('common','private_key')
		else:
			return False
		hl = hashlib.md5()
		t = int(time.time())
		hl.update(str("%s_%s" % (private_key,t)).encode(encoding='utf-8'))
		digest = hl.hexdigest()
		
		post_dict = {}
		post_dict["timestamp"] = "%s" % t
		post_dict["pkey"] = "%s" % digest
		post_dict["data"] = str(data)

		post_data = json.dumps(post_dict)
		#print post_data
		f = requests.post(
			url = url,
			data = post_data
		)
		print f.text
		#result = json.dumps
	except Exception,e:
		traceback.print_exc()

if __name__ == '__main__':
	client("http://192.168.73.111:12345/highriskcheck")
