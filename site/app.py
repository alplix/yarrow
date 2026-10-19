"""Yarrow's project page (yarrow.athena.org.tr): a plain static site, no accounts, no API,
nothing dynamic server-side. Run:  uvicorn app:app --port 8779   (see site/yarrow-site.service)

The page is built in the yarrow repo (site/build.py); this directory holds the built copy served at
yarrow.athena.org.tr. Re-run the build and copy the output after any content change, then redeploy.

Kept as its own tiny service (one service per tool, like iris-site, lupinus-site, rhizome-site and wollemi-site) so it can be
updated and restarted independently and never shares a process with anything that handles accounts or secrets.
"""
from __future__ import annotations

import pathlib

from fastapi import FastAPI, Request
from fastapi.responses import RedirectResponse
from fastapi.staticfiles import StaticFiles

STATIC = pathlib.Path(__file__).parent

app = FastAPI(title="Yarrow", docs_url=None, redoc_url=None, openapi_url=None)
CSP = ("default-src 'self'; script-src 'self' https://cdn.jsdelivr.net https://static.cloudflareinsights.com; "
       # the cloudflareinsights host is Cloudflare's own automatic RUM beacon, injected at the edge for any
       # proxied zone -- not something this app adds or can turn off from here, just allow-listed so it
       # doesn't show up as a (harmless) CSP violation in the console.
       "style-src 'self' 'unsafe-inline'; img-src 'self'; connect-src 'self' https://cloudflareinsights.com; base-uri 'none'; form-action 'none'; frame-ancestors 'none'")


@app.middleware("http")
async def security_headers(request: Request, call_next):
    resp = await call_next(request)
    resp.headers.update({"Content-Security-Policy": CSP, "X-Content-Type-Options": "nosniff",
                         "Referrer-Policy": "strict-origin-when-cross-origin", "X-Frame-Options": "DENY",
                         "Permissions-Policy": "camera=(), microphone=(), geolocation=()"})
    if request.url.path.endswith((".png", ".jpg", ".gif", ".ico")):
        resp.headers["Cache-Control"] = "public, max-age=604800, immutable"
    elif request.url.path.endswith((".html", "/")) or request.url.path == "":
        resp.headers["Cache-Control"] = "public, max-age=300"
    elif request.url.path.endswith((".js", ".obj", ".mtl", ".svg")):
        # short-lived on purpose: without an explicit Cache-Control, Cloudflare's default edge caching
        # still caches these extensions by its own heuristic, and a fix here can otherwise sit stale at
        # the edge for a while after a redeploy with no way to tell from the origin logs.
        resp.headers["Cache-Control"] = "public, max-age=300, must-revalidate"
    if request.url.scheme == "https" or request.headers.get("x-forwarded-proto") == "https":
        resp.headers["Strict-Transport-Security"] = "max-age=31536000"
    return resp


@app.exception_handler(404)
async def not_found(request: Request, exc):
    return RedirectResponse("/", status_code=302)


app.mount("/", StaticFiles(directory=str(STATIC), html=True), name="static")
