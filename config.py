# -*- coding=utf-8 -*-
import ConfigParser
import sys

reload(sys)
sys.setdefaultencoding('utf-8')

g_config_parser = ConfigParser.ConfigParser()
g_config_parser.read("./uni.conf")
