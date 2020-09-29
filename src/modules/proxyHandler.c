/*
    proxyHandler.c -- Proxy handler

    The proxy modules supports launching backend applications and connecting to pre-existing applications.
    It will multiplex multiple simultaneous requests to one or more apps.

    <Route /proxy>
        Reset pipeline
        CanonicalName https://example.com
        SetHandler proxyHandler
        Prefix /proxy
        ProxyConnect 127.0.0.1:9991 launch="program args" min=1 max=2 count=500 timeout=5mins multiplex=1
        # count is maximum number of requests, min/max are number of proxies
    </Route>

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/*********************************** Includes *********************************/

#include    "appweb.h"

#if ME_COM_PROXY && ME_UNIX_LIKE
/************************************ Locals ***********************************/

#define PROXY_DEBUG              1           //  For debugging (keeps files open in Proxy for debug output)

/*
    Default constants
 */
#define PROXY_MAX_PROXIES        1           //  Max of one proxy
#define PROXY_MIN_PROXIES        1           //  Min of one proxy (keep running after started)
#define PROXY_MAX_REQUESTS       MAXINT64    //  Max number of requests per proxy instance
#define PROXY_MAX_MULTIPLEX      1           //  Max number of concurrent requests per proxy instance
#define PROXY_PACKET_SIZE        8           //  Size of minimal Proxy packet

#define PROXY_Q_SIZE             ((PROXY_PACKET_SIZE + 65535 + 8) * 2)

#ifndef PROXY_WAIT_TIMEOUT
#define PROXY_WAIT_TIMEOUT       (30 * TPS)  //  Time to wait for a proxy
#endif
#ifndef PROXY_CONNECT_TIMEOUT
#define PROXY_CONNECT_TIMEOUT    (10 * TPS)  //  Time to wait for Proxy to respond to a connect
#endif
#ifndef PROXY_PROXY_TIMEOUT
#define PROXY_PROXY_TIMEOUT      (300 * TPS) //  Default inactivity time to preserve idle proxy
#endif
#ifndef PROXY_WATCHDOG_TIMEOUT
#define PROXY_WATCHDOG_TIMEOUT   (60 * TPS)  //  Frequence to check on idle proxies
#endif

/*
    Top level Proxy structure per route
 */
typedef struct Proxy {
    cchar           *endpoint;              //  App listening endpoint
    cchar           *launch;                //  Launch path
    cchar           *name;                  //  Proxy name
    int             multiplex;              //  Maximum number of requests to send to each app
    int             minApps;                //  Minumum number of proxies to maintain
    int             maxApps;                //  Maximum number of proxies to spawn
    uint64          maxRequests;            //  Maximum number of requests for launched apps before respawning
    MprTicks        proxyTimeout;           //  Timeout for an idle proxy to be maintained
    MprList         *apps;                  //  List of active apps
    MprList         *idleApps;              //  Idle apps
    MprMutex        *mutex;                 //  Multithread sync
    MprCond         *cond;                  //  Condition to wait for available app
    HttpLimits      *limits;                //  Proxy connection limits
    MprSsl          *ssl;                   //  SSL configuration
    MprEvent        *timer;                 //  Timer to check for idle apps
    HttpTrace       *trace;                 //  Tracing object for proxy side trace
    cchar           *ip;                    //  Listening IP address
    int             port;                   //  Listening port
    int             protocol;               //  HTTP/1 or HTTP/2 protocol
} Proxy;

/*
    Per app instance
 */
typedef struct ProxyApp {
    Proxy           *proxy;                 // Parent proxy pointer
    HttpTrace       *trace;                 // Default tracing configuration
    MprTicks        lastActive;             // When last active
    MprSignal       *signal;                // Mpr signal handler for child death
    bool            destroy;                // Must destroy app
    int             inUse;                  // In use counter
    int             pid;                    // Process ID of the app
    uint64          nextID;                 // Next request ID for this app
    MprList         *networks;              // Open network connections
    MprList         *requests;              // Current requests
} ProxyApp;

/*
    Per request instance
 */
typedef struct ProxyRequest {
    Proxy           *proxy;                 // Parent proxy pointer
    ProxyApp        *app;                   // Owning app
    HttpStream      *stream;                // Client request stream
    HttpNet         *proxyNet;              // Network to the proxy backend
    HttpStream      *proxyStream;           // Stream to the proxy backend for the current request
    HttpTrace       *trace;                 // Default tracing configuration
} ProxyRequest;

/*********************************** Forwards *********************************/

static Proxy *allocProxy(HttpRoute *route);
static ProxyApp *allocProxyApp(Proxy *proxy);
static ProxyRequest *allocProxyRequest(ProxyApp *app, HttpNet *net, HttpStream *stream);
static void closeProxy(HttpQueue *q);
static Proxy *getProxy(HttpRoute *route);
static ProxyApp *getProxyApp(Proxy *proxy, HttpStream *stream);
static HttpNet *getProxyNetwork(ProxyApp *app, MprDispatcher *dispatcher);
static void killProxyApp(ProxyApp *app);
static void manageProxy(Proxy *proxy, int flags);
static void manageProxyApp(ProxyApp *app, int flags);
static void manageProxyRequest(ProxyRequest *proxyRequest, int flags);
static int openProxy(HttpQueue *q);
static void proxyCleanRequest(ProxyRequest *req);
static int proxyCloseConfigDirective(MaState *state, cchar *key, cchar *value);
static int proxyConfigDirective(MaState *state, cchar *key, cchar *value);
static int proxyConnectDirective(MaState *state, cchar *key, cchar *value);
static void proxyFrontNotifier(HttpStream *stream, int event, int arg);
static int proxyLogDirective(MaState *state, cchar *key, cchar *value);
static int proxyTraceDirective(MaState *state, cchar *key, cchar *value);
static HttpStream *proxyCreateStream(ProxyRequest *req);
static void proxyIncoming(HttpQueue *q, HttpPacket *packet);
static void proxyBackNotifier(HttpStream *stream, int event, int arg);
static void proxyReadResponse(HttpQueue *q);
static void reapSignalHandler(ProxyApp *app, MprSignal *sp);
static ProxyApp *startProxyApp(Proxy *proxy, HttpStream *stream);
static void terminateIdleProxyApps(Proxy *proxy);
static void transferClientHeaders(HttpStream *stream, HttpStream *proxyStream);
static void transferProxyHeaders(HttpStream *proxyStream, HttpStream *stream);

/************************************* Code ***********************************/
/*
    Loadable module initialization
 */
PUBLIC int httpProxyInit(Http *http, MprModule *module)
{
    HttpStage   *handler;
    /*
        Add configuration file directives
     */
    maAddDirective("</ProxyConfig", proxyCloseConfigDirective);
    maAddDirective("<ProxyConfig", proxyConfigDirective);
    maAddDirective("ProxyConnect", proxyConnectDirective);
    maAddDirective("ProxyLog", proxyLogDirective);
    maAddDirective("ProxyTrace", proxyTraceDirective);

    /*
        Create Proxy handler to respond to client requests
     */
    if ((handler = httpCreateHandler("proxyHandler", module)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    http->proxyHandler = handler;
    handler->close = closeProxy;
    handler->open = openProxy;
    handler->incoming = proxyIncoming;
    return 0;
}


/*
    Open the proxyHandler for a new client request
 */
static int openProxy(HttpQueue *q)
{
    Http            *http;
    HttpNet         *proxyNet;
    HttpStream      *stream;
    Proxy           *proxy;
    ProxyApp        *app;
    ProxyRequest    *req;

    stream = q->stream;
    http = stream->http;

    /*
        Get a Proxy instance for this route. First time, this will allocate a new Proxy instance. Second and
        subsequent times, will reuse the existing instance.
     */
    proxy = getProxy(stream->rx->route);

    /*
        Get a ProxyApp instance. This will reuse an existing Proxy app if possible. Otherwise,
        it will launch a new Proxy app if within limits. Otherwise it will wait until one becomes available.
     */
    if ((app = getProxyApp(proxy, stream)) == 0) {
        httpError(stream, HTTP_CODE_SERVICE_UNAVAILABLE, "Cannot allocate ProxyApp for route %s", stream->rx->route->pattern);
        return MPR_ERR_CANT_OPEN;
    }

    /*
        Get or allocate a network connection to the proxy
     */
    if ((proxyNet = getProxyNetwork(app, stream->dispatcher)) == 0) {
        httpError(stream, HTTP_CODE_SERVICE_UNAVAILABLE, "Cannot allocate network for proxy %s", stream->rx->route->pattern);
        return MPR_ERR_CANT_OPEN;
    }

    /*
        Allocate a per-request instance
     */
    if ((req = allocProxyRequest(app, proxyNet, stream)) == 0) {
        httpError(stream, HTTP_CODE_SERVICE_UNAVAILABLE, "Cannot allocate proxy request for proxy %s", stream->rx->route->pattern);
        httpDestroyNet(proxyNet);
        return MPR_ERR_CANT_OPEN;
    }
    q->queueData = q->pair->queueData = req;

    httpEnableNetEvents(proxyNet);
    transferClientHeaders(stream, req->proxyStream);
    httpSetStreamNotifier(stream, proxyFrontNotifier);
    return 0;
}


/*
    Release a proxy app and request when the request completes. This closes the connection to the Proxy app.
    It will destroy the Proxy app on errors or if the number of requests exceeds the maxRequests limit.
 */
static void closeProxy(HttpQueue *q)
{
    Proxy           *proxy;
    ProxyRequest    *req;
    ProxyApp        *app;
    cchar           *msg;

    req = q->queueData;
    proxy = req->proxy;
    app = req->app;

    mprRemoveItem(app->requests, req);

    lock(proxy);
    if (--app->inUse <= 0) {
        if (mprRemoveItem(proxy->apps, app) < 0) {
            httpLogProc(proxy->trace, "proxy", "error", 0, "msg:Cannot find proxy app in list");
        }
        if (app->destroy || (proxy->maxRequests < MAXINT64 && app->nextID >= proxy->maxRequests) ||
                (mprGetListLength(proxy->apps) + mprGetListLength(proxy->idleApps) >= proxy->minApps)) {
            msg = "Destroy Proxy app";
            killProxyApp(app);
        } else {
            msg = "Release Proxy app";
            app->lastActive = mprGetTicks();
            mprAddItem(proxy->idleApps, app);
        }
        httpLogProc(proxy->trace, "proxy", "context", 0,
            "msg:%s, pid:%d, idle:%d, active:%d, id:%lld, maxRequests:%lld, destroy:%d, nextId:%lld",
            msg, app->pid, mprGetListLength(proxy->idleApps), mprGetListLength(proxy->apps),
            app->nextID, proxy->maxRequests, app->destroy, app->nextID);
        mprSignalCond(proxy->cond);
    }
    unlock(proxy);
}


static Proxy *allocProxy(HttpRoute *route)
{
    Proxy    *proxy;

    proxy = mprAllocObj(Proxy, manageProxy);
    proxy->name = sclone(route->pattern);
    proxy->apps = mprCreateList(0, 0);
    proxy->idleApps = mprCreateList(0, 0);
    proxy->mutex = mprCreateLock();
    proxy->cond = mprCreateCond();
    proxy->multiplex = PROXY_MAX_MULTIPLEX;
    proxy->maxRequests = PROXY_MAX_REQUESTS;
    proxy->minApps = PROXY_MIN_PROXIES;
    proxy->maxApps = PROXY_MAX_PROXIES;
    proxy->ip = sclone("127.0.0.1");
    proxy->port = 0;
    proxy->protocol = -1;
    proxy->proxyTimeout = PROXY_PROXY_TIMEOUT;
    proxy->ssl = NULL;
    proxy->trace = httpCreateTrace(route->trace);
    proxy->timer = mprCreateTimerEvent(NULL, "proxy-watchdog", PROXY_WATCHDOG_TIMEOUT,
        terminateIdleProxyApps, proxy, MPR_EVENT_QUICK);
    return proxy;
}


static void manageProxy(Proxy *proxy, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(proxy->apps);
        mprMark(proxy->cond);
        mprMark(proxy->endpoint);
        mprMark(proxy->idleApps);
        mprMark(proxy->ip);
        mprMark(proxy->launch);
        mprMark(proxy->limits);
        mprMark(proxy->name);
        mprMark(proxy->mutex);
        mprMark(proxy->ssl);
        mprMark(proxy->timer);
        mprMark(proxy->trace);
    }
}


static void terminateIdleProxyApps(Proxy *proxy)
{
    ProxyApp    *app;
    MprTicks    now;
    int         count, next;

    now = mprGetTicks();

    lock(proxy);
    count = mprGetListLength(proxy->apps) + mprGetListLength(proxy->idleApps);

    for (ITERATE_ITEMS(proxy->idleApps, app, next)) {
        if (app->pid && ((now - app->lastActive) > proxy->proxyTimeout)) {
            if (count-- > proxy->minApps) {
                killProxyApp(app);
            }
        }
    }
    unlock(proxy);
}


/*
    Get the proxy structure for a route and save in "eroute". Allocate if required.
    One Proxy instance is shared by all using the route.
 */
static Proxy *getProxy(HttpRoute *route)
{
    Proxy        *proxy;

    if ((proxy = route->eroute) == 0) {
        mprGlobalLock();
        if ((proxy = route->eroute) == 0) {
            proxy = route->eroute = allocProxy(route);
            proxy->trace = route->trace;
        }
        mprGlobalUnlock();
    }
    return proxy;
}


/*
    Notifier for events relating to the client (browser)
    Confusingly this is the server side
 */
static void proxyFrontNotifier(HttpStream *stream, int event, int arg)
{
    HttpNet         *net;
    ProxyRequest    *req;

    net = stream->net;
    assert(net->endpoint);

    // mprLog("proxyFrontNotifier", 0, "Proxy Front Event %d, state %d", event, stream->state);
    if ((req = stream->writeq->queueData) == 0) {
        return;
    }
    switch (event) {
    case HTTP_EVENT_READABLE:
    case HTTP_EVENT_WRITABLE:
        break;
    case HTTP_EVENT_ERROR:
        break;
    case HTTP_EVENT_DESTROY:
        break;

    case HTTP_EVENT_STATE:
        switch (stream->state) {
        case HTTP_STATE_BEGIN:
        case HTTP_STATE_CONNECTED:
        case HTTP_STATE_FIRST:
            break;
        case HTTP_STATE_PARSED:
            break;

        case HTTP_STATE_CONTENT:
            if (stream->rx->upgrade) {
                stream->rx->remainingContent = HTTP_UNLIMITED;
                req->proxyStream->upgraded = 1;
                req->proxyStream->keepAliveCount = 0;
                //  Force headers to be sent to mproxy
                httpPutPacketToNext(req->proxyStream->writeq, httpCreateDataPacket(0));
                httpServiceNetQueues(req->proxyStream->net, 0);
            }
            break;
        case HTTP_STATE_READY:
            if (!stream->rx->upgrade) {
                httpFinalizeOutput(req->proxyStream);
                httpServiceNetQueues(req->proxyStream->net, 0);
            }
            break;
        case HTTP_STATE_RUNNING:
        case HTTP_STATE_FINALIZED:
            break;
        case HTTP_STATE_COMPLETE:
            break;
        }
        break;
    }
}


/*
    Events for communications with the proxy app
    Confusingly this is using client side HTTP. Note that stream is the ProxyStream
 */
static void proxyBackNotifier(HttpStream *proxyStream, int event, int arg)
{
    ProxyRequest    *req, *rq;
    HttpNet         *net;
    int             complete, next;

    net = proxyStream->net;
    assert(net->endpoint == 0);

    if ((req = proxyStream->writeq->queueData) == 0) {
        return;
    }
    // mprLog("proxyBackNotifier", 0, "Proxy Back Event %d, state %d", event, proxyStream->state);
    complete = 0;
    switch (event) {
    case HTTP_EVENT_READABLE:
        proxyReadResponse(proxyStream->readq);
        break;

    case HTTP_EVENT_DONE:
        httpLog(proxyStream->trace, "tx.proxy", "result", "msg:Request complete");
        httpDestroyStream(proxyStream);
        net->dispatcher = mprCreateDispatcher("proxy", MPR_DISPATCHER_AUTO);
        net->autoDestroy = 1;
        mprPushItem(req->app->networks, net);
        httpEnableNetEvents(net);
        break;
    case HTTP_EVENT_WRITABLE:
        break;
    case HTTP_EVENT_ERROR:
        for (ITERATE_ITEMS(req->app->requests, rq, next)) {
            mprCreateEvent(req->stream->dispatcher, "proxy-reap", 0, proxyCleanRequest, req, 0);
        }
        break;
    case HTTP_EVENT_DESTROY:
        break;

    case HTTP_EVENT_STATE:
        switch (proxyStream->state) {
        case HTTP_STATE_BEGIN:
        case HTTP_STATE_CONNECTED:
        case HTTP_STATE_FIRST:
            break;
        case HTTP_STATE_PARSED:
            transferProxyHeaders(proxyStream, req->stream);
            break;
        case HTTP_STATE_CONTENT:
        case HTTP_STATE_READY:
            break;
        case HTTP_STATE_RUNNING:
        case HTTP_STATE_FINALIZED:
            break;
        case HTTP_STATE_COMPLETE:
            break;
        }
        break;
    }
}


/*
    Incoming data from the client destined for proxy.
 */
static void proxyIncoming(HttpQueue *q, HttpPacket *packet)
{
    HttpStream      *stream;
    ProxyRequest    *req;

    assert(q);
    assert(packet);
    stream = q->stream;

    if ((req = q->queueData) == 0) {
        return;
    }
    packet->stream = req->proxyStream;

    if (httpGetPacketLength(packet) == 0) {
        /* END Packet -- End of input */
        httpFinalizeInput(stream);
        if (stream->rx->remainingContent > 0) {
            httpError(stream, HTTP_CODE_BAD_REQUEST, "Client supplied insufficient body data");
        } else {
            httpFinalizeOutput(req->proxyStream);
        }
    } else {
        httpPutPacket(req->proxyStream->writeq, packet);
    }
    httpServiceNetQueues(req->proxyStream->net, 0);
}


static void proxyReadResponse(HttpQueue *q)
{
    HttpPacket      *packet;
    HttpStream      *stream;
    ProxyRequest    *req;

    req = q->queueData;
    if (req == 0) {
        return;
    }
    //  Stream back to the client
    stream = req->stream;
    for (packet = httpGetPacket(q); packet; packet = httpGetPacket(q)) {
        if (!httpWillQueueAcceptPacket(q, stream->writeq, packet)) {
            httpPutBackPacket(q, packet);
            return;
        }
        httpPutPacket(stream->writeq, packet);
        if (packet->flags & HTTP_PACKET_END) {
            httpFinalizeOutput(stream);
        }
    }
    httpServiceNetQueues(stream->net, 0);
}


/*
    Transfer response headers from the proxy to the client
 */
static void transferProxyHeaders(HttpStream *proxyStream, HttpStream *stream)
{
    MprKey      *kp;
    HttpUri     *loc, *uri;
    cchar       *location;

    assert(stream);
    assert(proxyStream);

    httpLog(proxyStream->trace, "rx.proxy", "result", "msg:Received headers from proxy");

    for (ITERATE_KEYS(proxyStream->rx->headers, kp)) {
        httpSetHeaderString(stream, kp->key, kp->data);
    }
    httpRemoveHeader(stream, "Connection");
    httpRemoveHeader(stream, "Keep-Alive");
    httpRemoveHeader(stream, "Content-Length");
    httpRemoveHeader(stream, "Transfer-Encoding");
    httpSetHeaderString(stream, "X-Proxied", "true");

    /*
        Modify the redirection location to use the Canonical domain or if not defined, the Host header from the request.
     */
    if ((location = httpGetTxHeader(stream, "Location")) != 0) {
        loc = httpCreateUri(location, 0);
        if (stream->rx->route->canonical) {
            uri = httpCloneUri(stream->rx->route->canonical, 0);
        } else {
            uri = httpCloneUri(stream->rx->parsedUri, 0);
        }
        uri->path = sjoin(stream->rx->route->prefix, loc->path, NULL);
        uri->ext = loc->ext;
        uri->reference = loc->reference;
        uri->query = loc->query;
        location = httpUriToString(uri, HTTP_COMPLETE_URI);
        httpSetHeaderString(stream, "Location", location);
    }
    httpSetStatus(stream, proxyStream->rx->status);
    if (proxyStream->rx->status == HTTP_CODE_SWITCHING) {
        proxyStream->rx->remainingContent = HTTP_UNLIMITED;
        stream->keepAliveCount = 0;
        stream->upgraded = 1;
        httpSetHeaderString(stream, "Connection", "Upgrade");
        //  Force headers to be sent to client
        httpPutPacketToNext(stream->writeq, httpCreateDataPacket(0));
        httpServiceNetQueues(stream->net, 0);
    }
}


/*
    Transfer request headers from the client to the proxy
 */
static void transferClientHeaders(HttpStream *stream, HttpStream *proxyStream)
{
    MprKey      *kp;
    HttpHost    *host;

    assert(stream);
    assert(proxyStream);

    host = stream->host;

    for (ITERATE_KEYS(stream->rx->headers, kp)) {
        httpSetHeaderString(proxyStream, kp->key, kp->data);
    }
    httpRemoveHeader(proxyStream, "Connection");
    httpRemoveHeader(proxyStream, "Content-Length");
    httpRemoveHeader(proxyStream, "Transfer-Encoding");
    httpSetHeaderString(proxyStream, "X-Client", stream->net->ip);
    httpSetHeaderString(stream, "X-Proxied", "true");

    if (stream->rx->route->canonical) {
        httpSetHeaderString(proxyStream, "X-Canonical", httpUriToString(stream->rx->route->canonical, HTTP_COMPLETE_URI));
    } else if (host->hostname) {
        httpSetHeaderString(proxyStream, "X-Hostname", host->hostname);
    }
}

/************************************************ ProxyApp ***************************************************************/
/*
    The ProxyApp represents the connection to a single Proxy app instance
 */
static ProxyApp *allocProxyApp(Proxy *proxy)
{
    ProxyApp   *app;

    app = mprAllocObj(ProxyApp, manageProxyApp);
    app->proxy = proxy;
    app->trace = proxy->trace;
    app->requests = mprCreateList(0, 0);
    app->networks = mprCreateList(0, 0);
    app->nextID = 1;
    return app;
}


static void manageProxyApp(ProxyApp *app, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(app->networks);
        mprMark(app->proxy);
        mprMark(app->requests);
        mprMark(app->signal);
        mprMark(app->trace);
    }
}


static ProxyApp *getProxyApp(Proxy *proxy, HttpStream *stream)
{
    ProxyApp    *app;
    MprTicks    timeout;
    int         idle, next;

    app = NULL;
    timeout = mprGetTicks() +  PROXY_WAIT_TIMEOUT;

    lock(proxy);
    /*
        Locate a ProxyApp to serve the request. Use an idle proxy app first. If none available, start a new proxy app
        if under the limits. Otherwise, wait for one to become available.
     */
    while (!app && mprGetTicks() < timeout) {
        idle = mprGetListLength(proxy->idleApps);
        if (idle > 0) {
            app = mprGetFirstItem(proxy->idleApps);
            mprRemoveItemAtPos(proxy->idleApps, 0);
            mprAddItem(proxy->apps, app);
            break;

        } else if (mprGetListLength(proxy->apps) < proxy->maxApps) {
            if ((app = startProxyApp(proxy, stream)) != 0) {
                mprAddItem(proxy->apps, app);
            }
            break;

        } else {
            for (ITERATE_ITEMS(proxy->apps, app, next)) {
                if (mprGetListLength(app->requests) < proxy->multiplex) {
                    break;
                }
            }
            if (app) {
                break;
            }
            unlock(proxy);
            //  TEST
            if (mprWaitForCond(proxy->cond, TPS) < 0) {
                return NULL;
            }
            lock(proxy);
        }
    }
    if (app) {
        app->lastActive = mprGetTicks();
        app->inUse++;
    }
    unlock(proxy);
    return app;
}


static HttpNet *getProxyNetwork(ProxyApp *app, MprDispatcher *dispatcher)
{
    HttpNet     *net;
    Proxy       *proxy;
    MprTicks    timeout;
    int         connected, backoff, protocol;

    proxy = app->proxy;

    while ((net = mprPopItem(app->networks)) != 0) {
        if (net->destroyed) {
            continue;
        }
        //  Switch to the client dispatcher to serialize requests (no locking yea!)
        mprDestroyDispatcher(net->dispatcher);
        net->dispatcher = dispatcher;
        net->autoDestroy = 0;
        return net;
    }

    protocol = proxy->protocol;
    if (protocol >= 2 && !proxy->ssl) {
        mprLog("proxy error", 0, "Cannot use HTTP/2 when SSL is not configured inside ProxyConfig");
        protocol = 1;
    }

    if ((net = httpCreateNet(dispatcher, NULL, protocol, HTTP_NET_ASYNC)) == 0) {
        return net;
    }
    net->limits = httpCloneLimits(proxy->limits);

    timeout = mprGetTicks() + PROXY_CONNECT_TIMEOUT;
    connected = 0;
    backoff = 1;

    mprAddRoot(net);

    while (1) {
        if (httpConnectNet(net, proxy->ip, proxy->port, proxy->ssl) == 0) {
            connected = 1;
            mprLog("proxy", 2, "Proxy connected to %s:%d", proxy->ip, proxy->port);
            break;
        }
        net->eof = net->error = 0;
        if (mprGetTicks() >= timeout) {
            break;
        }
        //  WARNING: yields
        mprSleep(backoff);
        backoff = backoff * 2;
        if (backoff > 50) {
            mprLog("proxy", 2, "Proxy retry connect to %s:%d", proxy->ip, proxy->port);
            if (backoff > 2000) {
                backoff = 2000;
            }
        }
    }
    mprRemoveRoot(net);

    if (!connected) {
        mprLog("proxy error", 0, "Cannot connect to proxy %s at %s:%d", proxy->name, proxy->ip, proxy->port);
        httpDestroyNet(net);
        net = 0;
    }
    return net;
}


/*
    Start a new Proxy app process. Called with lock(proxy)
 */
static ProxyApp *startProxyApp(Proxy *proxy, HttpStream *stream)
{
    ProxyApp    *app;
    HttpRoute   *route;
    cchar       **argv, *command;
    int         i;

    route = stream->rx->route;
    app = allocProxyApp(proxy);

    if (proxy->launch) {
        mprMakeArgv(proxy->launch, &argv, 0);
        command = argv[0];

        httpLogProc(app->trace, "proxy", "context", 0, "msg:Start Proxy app, command:%s", command);

        if (!app->signal) {
            app->signal = mprAddSignalHandler(SIGCHLD, reapSignalHandler, app, NULL, MPR_SIGNAL_BEFORE);
        }
        if ((app->pid = fork()) < 0) {
            fprintf(stderr, "Fork failed for Proxy");
            return NULL;

        } else if (app->pid == 0) {
            /*
                CHILD: When debugging, keep stdout/stderr open so printf/fprintf from the Proxy app will show in the console.
             */
            for (i = PROXY_DEBUG ? 3 : 1; i < 128; i++) {
                close(i);
            }
            // FUTURE envp[0] = sfmt("FCGI_WEB_SERVER_ADDRS=%s", route->host->defaultEndpoint->ip);
            if (execve(command, (char**) argv, NULL /* (char**) &env->items[0] */) < 0) {
                printf("Cannot exec proxy app: %s\n", command);
            }
            return NULL;
        } else {
            httpLogProc(app->trace, "proxy", "context", 0, "msg:Proxy started, command:%s, pid:%d", command, app->pid);
        }
    }
    return app;
}


/*
    Proxy process has died, so reap the status and inform relevant streams.
    WARNING: this may be called before all the data has been read from the socket, so we must not set eof = 1 here.
    WARNING: runs on the MPR dispatcher. Everyting must be "proxy" locked.
 */
static void reapSignalHandler(ProxyApp *app, MprSignal *sp)
{
    Proxy           *proxy;
    ProxyRequest    *req;
    int             next, status;

    proxy = app->proxy;

    lock(proxy);
    if (app->pid && waitpid(app->pid, &status, WNOHANG) == app->pid) {
        httpLogProc(app->trace, "proxy", WEXITSTATUS(status) == 0 ? "context" : "error", 0,
            "msg:Proxy exited, pid:%d, status:%d", app->pid, WEXITSTATUS(status));
        if (app->signal) {
            mprRemoveSignalHandler(app->signal);
            app->signal = 0;
        }
        if (mprLookupItem(app->proxy->idleApps, app) >= 0) {
            mprRemoveItem(app->proxy->idleApps, app);
        }
        app->destroy = 1;
        app->pid = 0;

        /*
            Notify all requests on their relevant dispatcher
         */
        for (ITERATE_ITEMS(app->requests, req, next)) {
            mprCreateEvent(req->stream->dispatcher, "proxy-reap", 0, proxyCleanRequest, req, 0);
        }
    }
    unlock(proxy);
}


/*
    Clean / abort request
 */
static void proxyCleanRequest(ProxyRequest *req)
{
    HttpStream  *stream;

    stream = req->stream;

    if (stream->state <= HTTP_STATE_BEGIN || stream->rx->route == NULL) {
        /* Request already complete and stream has been recycled (prepared for next request) */
        return;
    }
    httpError(stream, HTTP_CODE_INTERNAL_SERVER_ERROR, "Proxy comms error");
    httpProcess(stream->inputq);
}


/*
    Kill the Proxy app due to error or maxRequests limit being exceeded
 */
static void killProxyApp(ProxyApp *app)
{
    lock(app->proxy);
    if (app->pid) {
        httpLogProc(app->trace, "proxy", "context", 0, "msg:Kill Proxy process, pid:%d", app->pid);
        if (app->pid) {
            kill(app->pid, SIGTERM);
        }
    }
    unlock(app->proxy);
}

/************************************************ ProxyRequest ***********************************************************/

static ProxyRequest *allocProxyRequest(ProxyApp *app, HttpNet *proxyNet, HttpStream *stream)
{
    ProxyRequest    *req;

    req = mprAllocObj(ProxyRequest, manageProxyRequest);
    req->stream = stream;
    req->trace = app->trace;
    req->proxy = app->proxy;
    req->app = app;
    req->proxyNet = proxyNet;
    if ((req->proxyStream = proxyCreateStream(req)) == 0) {
        return 0;
    }
    mprAddItem(app->requests, req);
    return req;
}


static HttpStream *proxyCreateStream(ProxyRequest *req)
{
    HttpStream      *proxyStream, *stream;
    HttpRx          *rx;
    Proxy           *proxy;
    cchar           *prefix, *uri;

    stream = req->stream;
    rx = stream->rx;
    proxy = req->proxy;

    if ((proxyStream = httpCreateStream(req->proxyNet, 0)) == 0) {
        return 0;
    }
    httpSetStreamNotifier(proxyStream, proxyBackNotifier);
    httpCreatePipeline(proxyStream);

    proxyStream->trace = proxy->trace;
    proxyStream->proxied = 1;
    proxyStream->writeq->queueData = proxyStream->readq->queueData = req;
    proxyStream->rx->route = stream->http->clientRoute;

    prefix = rx->route->prefix;
    uri = (prefix && sstarts(rx->uri, prefix)) ? &rx->uri[slen(prefix)] : rx->uri;
    proxyStream->tx->parsedUri = httpCreateUri(uri, 0);
    proxyStream->tx->parsedUri->query = rx->parsedUri->query;
    proxyStream->tx->method = stream->rx->method;
    proxyStream->limits->inactivityTimeout = proxy->proxyTimeout;

    httpSetState(proxyStream, HTTP_STATE_CONNECTED);
    return proxyStream;
}


static void manageProxyRequest(ProxyRequest *req, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(req->app);
        mprMark(req->proxy);
        mprMark(req->proxyNet);
        mprMark(req->stream);
        mprMark(req->trace);
    }
}


/*
    <ProxyConfig>
    </ProxyConfig>
 */
static int proxyConfigDirective(MaState *state, cchar *key, cchar *value)
{
    state = maPushState(state);
    if (state->enabled) {
        state->route = httpCreateInheritedRoute(state->route);
    }
    return 0;
}

/*
    </ProxyConfig>
 */
static int proxyCloseConfigDirective(MaState *state, cchar *key, cchar *value)
{
    Proxy   *proxy;

    proxy = getProxy(state->route);
    if (state->route != state->prev->route) {
        /*
            Extract SSL and limit configuration
         */
         if (proxy->protocol != 1) {
             proxy->ssl = state->route->ssl;
         }
         if (proxy->protocol < 0) {
             proxy->protocol = 1;
         }
         proxy->limits = state->route->limits;
    }
    maPopState(state);
    return 0;
}


static int proxyConnectDirective(MaState *state, cchar *key, cchar *value)
{
    Proxy   *proxy;
    cchar   *endpoint, *args;
    char    *option, *ovalue, *tok;

    proxy = getProxy(state->route);

    if (!maTokenize(state, value, "%S ?*", &endpoint, &args)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    proxy->endpoint = endpoint;
    proxy->ssl = state->route->ssl;

    if (args && *args) {
        for (option = maGetNextArg(sclone(args), &tok); option; option = maGetNextArg(tok, &tok)) {
            option = ssplit(option, " =\t,", &ovalue);
            ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);

            if (smatch(option, "count")) {
                proxy->maxRequests = httpGetNumber(ovalue);
                if (proxy->maxRequests < 1) {
                    proxy->maxRequests = 1;
                }

            } else if (smatch(option, "http1")) {
                proxy->protocol = 1;
                proxy->ssl = 0;

            } else if (smatch(option, "http2")) {
                proxy->protocol = 2;
                if (!proxy->ssl) {
                    proxy->ssl = mprCreateSsl(0);
                }
                mprSetSslAlpn(proxy->ssl, "h2");

            } else if (smatch(option, "launch")) {
                proxy->launch = sclone(httpExpandRouteVars(state->route, ovalue));

            } else if (smatch(option, "max")) {
                proxy->maxApps = httpGetInt(ovalue);
                if (proxy->maxApps < 1) {
                    proxy->maxApps = 1;
                }

            } else if (smatch(option, "min")) {
                proxy->minApps = httpGetInt(ovalue);
                if (proxy->minApps < 1) {
                    proxy->minApps = 1;
                }

            } else if (smatch(option, "multiplex")) {
                proxy->multiplex = httpGetInt(ovalue);
                if (proxy->multiplex < 1) {
                    proxy->multiplex = 1;
                }

            } else if (smatch(option, "ssl")) {
                if (!proxy->ssl) {
                    proxy->ssl = mprCreateSsl(0);
                }
                //verifyPeer

            } else if (smatch(option, "timeout")) {
                proxy->proxyTimeout = httpGetTicks(ovalue);
                if (proxy->proxyTimeout < (30 * TPS)) {
                    proxy->proxyTimeout = 30 * TPS;
                }
            } else {
                mprLog("proxy error", 0, "Unknown Proxy option %s", option);
                return MPR_ERR_BAD_SYNTAX;
            }
        }
    }
    state->route->ssl = proxy->ssl;

    /*
        Pre-test the endpoint
     */
    if (mprParseSocketAddress(proxy->endpoint, &proxy->ip, &proxy->port, NULL, 9128) < 0) {
        mprLog("proxy error", 0, "Cannot bind Proxy proxy address: %s", proxy->endpoint);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}


static int proxyLogDirective(MaState *state, cchar *key, cchar *value)
{
    Proxy   *proxy;

    proxy = getProxy(state->route);
    proxy->trace = httpCreateTrace(proxy->trace);
    proxy->trace->flags &= ~MPR_LOG_CMDLINE;
    return maTraceLogDirective(state, proxy->trace, key, value);
}


static int proxyTraceDirective(MaState *state, cchar *key, cchar *value)
{
    Proxy   *proxy;

    proxy = getProxy(state->route);
    proxy->trace->flags &= ~MPR_LOG_CMDLINE;
    return maTraceDirective(state, proxy->trace, key, value);
}

#endif /* ME_COM_PROXY */

/*
    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.
 */
