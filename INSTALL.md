# Server Installation
Run the interactive installer: `install.sh`. 

## config.ini
The install script will walk you through the construction of config.ini file.

It should look like this.

```
# Where your instance will be accessible at
hostname=fiy.yourdomain.com

# Where fediy is installed
data_dir=/opt/fediy

# Additional entropy source
salt=your_random_salt_here_change_this

# Port for protocol server to listen on
# It's expected that you use a reverse proxy
port=8848

# Number of IO threads to use (default: `nproc`)
concurrency=4
```

## SSL
The server should be reverse-proxied through a server that adds ssl.

For example, this nginx.conf should work:
```nginx.conf
    server {
        listen 80;
        listen [::]:80;
        server_name bodge.dev;
        server_name *.bodge.dev;
        access_log /var/log/nginx/fiy.access.log;
        location / {
            proxy_pass http://127.0.0.1:8848;
            proxy_set_header Host $host;
        }
    }
    server {
        listen 443 ssl;
        listen [::]:443 ssl;
        server_name bodge.dev;
        server_name *.bodge.dev;
        access_log /var/log/nginx/fiy.access.log;
        ssl_certificate  /etc/letsencrypt/live/bodge.dev-0001/fullchain.pem;
        ssl_certificate_key /etc/letsencrypt/live/bodge.dev-0001/privkey.pem;
        location / {
            proxy_pass http://127.0.0.1:8848;
            proxy_set_header Host $host;
        }
    }
```
Run install.sh.

## Security
Consider running the fediy server as a dedicated user that only has access to files in the install dir.

## Notes
- Your system clock must be accurate in order to interact with other instances

## TODO: Paths
Eventually these will be accurate, for now everything is in a provided install directory
- `/usr/bin/fediy` - portal/protcol server
- `/usr/share/fediy/config.ini`
- `/usr/share/fediy/templates/*.html`
- `/var/lib/fediy/db.db3` - database