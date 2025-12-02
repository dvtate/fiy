
require('dotenv').configure();

const express = require('express');

const app = express();

// TODO convert this into an express middleware/wrapper
const FIY = {
    id: 'fiy.demo',
    version: '0.0',
    domain: process.env.FIY_DOMAIN,
    dataDir: process.env.FIY_DATA_DIR,
    bearer_token_we_accept: '',
    bearer_token_we_send: '',
    baseUri: '',

    async now() {
        const protocol = FIY.domain.includes(':') ? 'http://' : 'https://';
        const r = await fetch(protocol + FIY.domain + '/peer/key');
        if (r.status !== 200)
            return -1;
        const ts = await r.text();
        return parseInt(r.headers.get('fiy'));
    },

    genToken() {
        const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_';
        let ret = '';
        for (let i = 0; i < 24; i++)
            ret += alphabet[Math.floor(Math.random() * alphabet.length)];
        return ret;
    },

    async start(req, res) {
        const body = JSON.parse(req.body);

        // Verify timestamp
        const protocol = body.base_uri.startsWith('https') ? 'https://' : 'http://';
        const r = await fetch( protocol + FIY.domain + '/peer/key');
        const now = parseInt(r.headers.get('fiy-now'));
        if (now < body.now) {
            res.status(401).send("provided now is in the future");
            return;
        }
        if (now - body.now > 60 * 3) {
            res.status(401).send("timestamp outdated");
            return;
        }

        // Verify signature
        const cert = await r.text();
        const sig = req.headers.get('fiy-signature');
        // TODO verify

        FIY.baseUri = body.base_uri;
        FIY.bearer_token_we_send = body.bearer;
        FIY.bearer_token_we_accept = FIY.genToken();

        const resp = {
            accept: FIY.bearer_token_we_accept,
            send: FIY.bearer_token_we_send,
            id: FIY.id,
            version: FIY.version,
        };
        res.json(resp);
    },

    async stop(req, res) {
        if (req.headers.get('fiy-auth') !== FIY.bearer_token_we_accept) {
            res.status(401).send('unauthenticated');
            return;
        }
        FIY.bearer_token_we_accept = '';
        FIY.bearer_token_we_send = '';
        FIY.baseUri = '';
        res.send('ok');
    },

    async login(user, password){
        if (!FIY.baseUri)
            return -1;

        // TODO encrypt
        const uri = (FIY.baseUri.startsWith('https://') ? 'https://' : 'http://')
            + FIY.domain + '/internal/login';

        const r = await fetch(uri, {
            method: 'GET',
            headers: {
                'Fiy-Auth': FIY.bearer_token_we_send,
                'Fiy-User': user,
                'Fiy-Pass': password,
            },
        });
        return r.status; // 200 on login, 401 on incorrect login, anything else is error
    },

    async userInfo(user) {
        if (!FIY.baseUri)
            return -1;
        const uri = (FIY.baseUri.startsWith('https://') ? 'https://' : 'http://')
            + FIY.domain + '/internal/user_info/' + user;
        const r = await fetch(uri, {
            headers: { 'Fiy-Auth': FIY.bearer_token_we_send },
        });
        return JSON.parse(await r.text());
    },

    async request({
        modId,
        domain,
        localUser,
        httpMethod,
        targetPath,
        headers,
        body,
    }){
        // Prefix/convert headers
        const userHeaders = {};
        if (typeof headers === 'string') {
            // Parse string headers
            headers.split('\n')
                .map(h => h.split(':'))
                .forEach(([h, v]) => {
                   h = h.trim();
                   if (h.length === 0)
                       return;
                   v = v.trim();
                   if (v.length === 0)
                       return;
                   userHeaders['Fiy-H-' + h] = v;
                });
            headers = userHeaders;
        } else {
            // Object of headers+values
            if (typeof headers !== 'object')
                throw new TypeError('headers should be a map of headers to values');
            Object.entries(headers).forEach(([h, v]) => {
                userHeaders['Fiy-H-' + h] = v;
            });
            headers = userHeaders;
        }

        // Send request to the protocol server
        const uri = (FIY.baseUri.startsWith('https://') ? 'https://' : 'http://')
            + FIY.domain + '/internal/mods/request';
        const r = await fetch(uri, {
            method: 'POST',
            headers: {
                'Fiy-Auth': FIY.bearer_token_we_send,
                'Fiy-Domain': domain,
                'Fiy-Mod': modId,
                'Fiy-User': localUser,
                'Fiy-Method': httpMethod,
                'Fiy-Path': targetPath,
                ...headers,
            },
        });

        return {
            status: r.status,
            body: await r.text(),
            headers: r.headers,
        };
    },

    async log(level, msg) {
        // TODO /internal/mods/log
    },
};

app.post('/start', FIY.start);
app.post('/stop', FIY.stop);

// Everything past here requires authentication
app.use(async (req, res, next) => {
    const token = req.headers.get('fiy-auth')
        || req.headers.get('Authentication');
    if (token !== FIY.bearer_token_we_accept) {
        res.status(401).send('unauthenticated');
        return;
    }
    next();
});

app.delete('/user', async (req, res) => {
    console.log('deleting user ' + req['fiy-user']);
    // ... delete any data associated with that user
});

app.use('/request', async (req, res, next) => {
    const user = req.headers.get('fiy-user');
    res.html("<h1>Welcome ${user ? (", " + user) : ''}</h1>");
});

app.get('/request/testing', async (req, res, next) => {
    res.send("Hello, @", req.headers.get('fiy-user'));
    console.log("userInfo: ", FIY.userInfo(req.headers.get('fiy-user')));
});