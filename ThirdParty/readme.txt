搭建RTMP服务器

1.nginx.org 下载最新安装包
2.进 github 搜 nginx rtmp,下载源码
3.配置
./configure --add-module=/path/to/nginx-rtmp-module --prefix=/usr/local/nginx

4.openssl出错，进openssl.org，下载编译openssl
./configure --add-module=/path/to/nginx-rtmp-module --prefix=/usr/local/nginx --with-openssl=../openssl

5.修改配置文件 nginx.conf
rtmp {
	server {
		listen 1935;
		chunk_size 4096;
		application live { #创建一个发布应用 live ,发布到该应用的地址就是：rtmp://ip地址:1935/live/ 
		    live on;
		    record off;
		}
	}
}