self.addEventListener("install", event => {
    event.waitUntil(self.skipWaiting());
});

self.addEventListener("message", event => {
    const title = event.data.title;
    const options = event.data.options;
    event.waitUntil(self.registration.showNotification(title, options));
});

self.addEventListener("notificationclick", event => {
    console.log("action:", event.action);
    let action = event.action;
    if (event.action === "" && event.notification.actions.length > 0) return;
    switch (event.notification.tag) {
        case "call":
            if (event.action === "") return;
            break;
        case "yo":
            action = "yo_ok";
            break;
    }

    event.waitUntil((async () => {
        const cs = await findClients();
        for (let i = 0; i < cs.length; i++) {
            const client = cs[i];
            if ("focus" in client) {
                await client.postMessage({ action });
                if (action !== "ng") {
                    await client.focus();
                }
                return;
            }
        }
        if ("openWindow" in clients) {
            const client = await clients.openWindow("/");
            return client.postMessage({ action });
        }
    })());
});

async function findClients() {
    const clientList = await clients.matchAll({ type: "window" });
    return clientList.filter(client => {
        return client.url.startsWith(self.location.origin);
    });
}
