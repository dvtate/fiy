# Instance Hosting Guide

## Installation
- Run `install.sh`.
- Start the FIY server / enable systemd service.

## Security Hardening
### Reverse Proxy + SSL
Because the FIY server does not provide support SSL it's very highly recommended to reverse proxy it behind nginx/apache
with an appropriate SSL configuration.

Example nginx.conf:
```
http {
        server {
                listen 80;
                listen [::]:80;
                server_name fiy.to *.fiy.to;
                access_log /var/log/nginx/fiy.access.log;
                return 301 https://$host$request_uri;
        }
        server {          
                listen 443 ssl;
                listen [::]:443 ssl;                  
                server_name fiy.to *.fiy.to;           
                access_log /var/log/nginx/fiy.access.log;

                ssl_certificate  /path/to/fullchain.pem;
                ssl_certificate_key     /path/to/privkey.pem;

                client_max_body_size 1G;

                http2 on;

                server_tokens off;

                location / {
                        proxy_pass http://127.0.0.1:8848;
                        proxy_set_header Host $host;
                }
        }
}
```

### Don't Run Server as Root
It's never recommended to run an internet-facing application as root. Instead you should create a dedicated user
with access restricted to only necessary paths (ie - /opt/fiy ).

## Scaling
See [SCALING.md](SCALING.md).