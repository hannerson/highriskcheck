# -*- coding=utf-8 -*-

import os,sys
import MySQLdb
import MySQLdb.cursors
import traceback
from DBUtils.PooledDB import PooledDB
from config import *

reload(sys)
sys.setdefaultencoding('utf-8')

###global###
try:
	###database###
	#g_pool_Src = PooledDB(MySQLdb,5,host=g_config_parser.get("dbconfig","src_dbhost"),user=g_config_parser.get("dbconfig","src_dbuser"),passwd=g_config_parser.get("dbconfig","src_dbpwd"),db=g_config_parser.get("dbconfig","src_dbname"),charset=g_config_parser.get("dbconfig","src_dbcharset"),port=g_config_parser.getint("dbconfig","src_dbport"))
	#g_pool_Src2 = PooledDB(MySQLdb,2,host=g_config_parser.get("dbconfig","src_dbhost2"),user=g_config_parser.get("dbconfig","src_dbuser2"),passwd=g_config_parser.get("dbconfig","src_dbpwd2"),db=g_config_parser.get("dbconfig","src_dbname2"),charset=g_config_parser.get("dbconfig","src_dbcharset2"),port=g_config_parser.getint("dbconfig","src_dbport2"))
	#g_pool_Res = PooledDB(MySQLdb,2,host=g_config_parser.get("dbconfig","res_dbhost"),user=g_config_parser.get("dbconfig","res_dbuser"),passwd=g_config_parser.get("dbconfig","res_dbpwd"),db=g_config_parser.get("dbconfig","res_dbname"),charset=g_config_parser.get("dbconfig","res_dbcharset"),port=g_config_parser.getint("dbconfig","res_dbport"))
	#g_pool_Run = PooledDB(MySQLdb,2,host=g_config_parser.get("dbconfig","task_dbhost"),user=g_config_parser.get("dbconfig","task_dbuser"),passwd=g_config_parser.get("dbconfig","task_dbpwd"),db=g_config_parser.get("dbconfig","task_dbname"),charset=g_config_parser.get("dbconfig","task_dbcharset"),port=g_config_parser.getint("dbconfig","task_dbport"))
	#g_pool_TMApi = PooledDB(MySQLdb,2,host=g_config_parser.get("dbconfig","TMApi_dbhost"),user=g_config_parser.get("dbconfig","TMApi_dbuser"),passwd=g_config_parser.get("dbconfig","TMApi_dbpwd"),db=g_config_parser.get("dbconfig","TMApi_dbname"),charset=g_config_parser.get("dbconfig","TMApi_dbcharset"),port=g_config_parser.getint("dbconfig","TMApi_dbport"))

	#g_pool_Relation = PooledDB(MySQLdb,2,host=g_config_parser.get("dbconfig","relation_dbhost"),user=g_config_parser.get("dbconfig","relation_dbuser"),passwd=g_config_parser.get("dbconfig","relation_dbpwd"),db=g_config_parser.get("dbconfig","relation_dbname"),charset=g_config_parser.get("dbconfig","relation_dbcharset"),port=g_config_parser.getint("dbconfig","relation_dbport"))
	###global###
	g_status = {"default":0,"success":1,"notindms":2}
except Exception,e:
	traceback.print_exc()
