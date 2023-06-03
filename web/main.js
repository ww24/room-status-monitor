import "@spectrum-web-components/bundle/elements.js";
import { Overlay } from "@spectrum-web-components/overlay";
import "./elements.js";

const historyTable = document.getElementById("history");

class BLE {
    constructor() {
        this.serviceUUID = "d3e6a1bb-2f35-4853-9f02-ba02b91044f1";
        this.characteristicStatusUUID = "e9229875-87d4-4b24-b703-361649ad9ad6";
        this.characteristicContentUUID = "c310ce58-d663-4dcd-9e37-8a5431f6550d";
        this.characteristicCallUUID = "5a8e96e4-f950-4512-9a50-1008c6629d43";
        this.options = {
            filters: [
                { services: [this.serviceUUID] }
            ]
        };
        this.device = null;
        this.autoReconnect = true;
        this.lastHistoryTimestamp = null;
    }

    get available() {
        return navigator.bluetooth.getAvailability().then(available => {
            if (!available) {
                return Promise.reject(new Error("not supported"));
            }
            return true;
        });
    }

    async pair() {
        if (this.device != null) {
            return this.device;
        }

        await this.available;
        const device = await navigator.bluetooth.requestDevice(this.options);
        this.device = device;

        const self = this;
        device.addEventListener("gattserverdisconnected", event => {
            disconnected();

            const device = event.target;
            if (!this.autoReconnect) {
                console.log(`Device ${device.name} is disconnected.`);
                return;
            }
            console.log(`Device ${device.name} is disconnected. Reconnect...`);
            self.connect()
                .then(() => {
                    return self.writeStatus(statusSelect.value);
                })
                .catch(err => {
                    console.error(`Something went wrong. ${err}`);
                });
        });
        return device;
    }

    async connect() {
        const device = await this.pair();
        if (device.gatt.connected) {
            return device.gatt;
        }

        const gatt = await device.gatt.connect();
        connected();
        console.log("connected!");
        console.log(`serviceUUID: ${this.serviceUUID}`);
        const characteristic = await this.characteristic(this.serviceUUID, this.characteristicCallUUID)(gatt);
        await characteristic.startNotifications();

        const self = this;
        characteristic.addEventListener("characteristicvaluechanged", event => {
            console.log(`size: ${event.target.value.byteLength} bytes`);
            const decoder = new TextDecoder();
            const text = decoder.decode(new Uint8Array(event.target.value.buffer));
            console.log(`value: ${text}`);
            const timestamp = new Date();
            self.lastHistoryTimestamp = timestamp;

            switch (text) {
                case "calling":
                    historyTable.addHistory({ time: timestamp, title: "Call" });
                    const optionCalling = {
                        tag: "call",
                        actions: [
                            {
                                action: "ok",
                                title: "対応可能"
                                // icon: ""
                            },
                            {
                                action: "ng",
                                title: "対応不可"
                                // icon: ""
                            },
                        ]
                    };
                    notify("呼び出しがありました\n対応できますか？", optionCalling).catch(err => {
                        console.error(`Something went wrong. ${err}`);
                    });
                    break;

                case "yo":
                    historyTable.addHistory({ time: timestamp, title: "Yo" });
                    const optionYo = { tag: "yo" };
                    notify("Yo されました", optionYo).catch(err => {
                        console.error(`Something went wrong. ${err}`);
                    });
                    break;
            }
        });
        return gatt;
    }

    async disconnect() {
        this.autoReconnect = false;
        if (this.device == null) {
            return false;
        }
        const device = await this.pair();
        if (!device.gatt.connected) {
            return false;
        }
        device.gatt.disconnect();
        return true;
    }

    service(service) {
        return (gatt) => gatt.getPrimaryService(service);
    }

    characteristic(service, characteristic) {
        return (gatt) => this.service(service)(gatt).then(service => service.getCharacteristic(characteristic));
    }

    async readStatus() {
        const characteristic = await this.connect().
            then(this.characteristic(this.serviceUUID, this.characteristicStatusUUID));
        return characteristic.readValue();
    }

    async writeStatus(status) {
        const characteristic = await this.connect().
            then(this.characteristic(this.serviceUUID, this.characteristicStatusUUID));
        const encoder = new TextEncoder();
        const arr = encoder.encode(status);
        return characteristic.writeValueWithResponse(arr);
    }

    async readContent() {
        const characteristic = await this.connect().
            then(this.characteristic(this.serviceUUID, this.characteristicContentUUID));
        return characteristic.readValue();
    }

    async writeContent(content) {
        const characteristic = await this.connect().
            then(this.characteristic(this.serviceUUID, this.characteristicContentUUID));
        const encoder = new TextEncoder();
        const arr = encoder.encode(content);
        return characteristic.writeValueWithResponse(arr);
    }

    async writeResponse(content) {
        let response = "OK";
        switch (content) {
            case messageSettings.response_busy:
                response = "Busy";
                break;
        }
        historyTable.updateHistoryResponse({ time: this.lastHistoryTimestamp, response });
        return this.writeContent(content);
    }
}

const ble = new BLE();

class DoubleValueStore {
    constructor(key, defaultValue = 0.0) {
        this.key = key;
        const v = localStorage.getItem(this.key);
        if (v == null) {
            this.value = defaultValue;
        }
        this.v = Number(v);
    }
    get value() {
        return this.v;
    }
    set value(v) {
        this.v = v;
        localStorage.setItem(this.key, v.toString())
    }
}
const gain = new DoubleValueStore("gain");

class TonePlayer {
    constructor(melody = []) {
        this.ctx = null;
        this.gain = null;
        this.melody = melody;
        this.toneSize = 0.35;
    }

    initialize() {
        if (this.ctx != null) { return; }
        // ユーザーアクションによる生成が必要
        this.ctx = new AudioContext();
        this.gain = this.ctx.createGain();
        this.gain.gain.value = gain.value;
        this.gain.connect(this.ctx.destination);
    }

    play() {
        if (this.ctx == null) {
            console.error("should call initialize method before play");
            return;
        }
        const self = this;
        const oscillator1 = this.ctx.createOscillator();
        const oscillator2 = this.ctx.createOscillator();
        oscillator1.type = "square";
        oscillator2.type = "square";
        oscillator1.connect(this.gain);
        oscillator2.connect(this.gain);
        this.melody.forEach((tone, i) => {
            const time = self.ctx.currentTime + self.toneSize * i;
            oscillator1.frequency.setValueAtTime(tone[0], time);
            oscillator2.frequency.setValueAtTime(tone[1] || 0, time);
        });
        oscillator1.start();
        oscillator2.start();
        const wait = this.toneSize * (this.melody.length + 1);
        oscillator1.stop(this.ctx.currentTime + wait);
        oscillator2.stop(this.ctx.currentTime + wait);
        setTimeout(() => {
            oscillator1.disconnect();
            oscillator2.disconnect();
        }, wait * 1000);
    }
}
const tone = new TonePlayer([
    [739],
    [583],
    [440, 369],
    [587],
    [659, 523],
    [880],
    [880],
    [440],
    [659, 440],
    [739],
    [659, 440],
    [440],
    [587, 369]
]);

const connectionStatus = document.getElementById("connection-status");
const connectBtn = document.getElementById("connect");
const disconnectBtn = document.getElementById("disconnect");

const statusSelect = document.getElementById("status");

const settingGainInput = document.getElementById("gain");

const callingDialog = document.getElementById("calling-dialog");
const yoDialog = document.getElementById("yo-dialog");

function connected() {
    connectBtn.setAttribute("disabled", true);
    disconnectBtn.removeAttribute("disabled");
    connectionStatus.setAttribute("variant", "positive");
    connectionStatus.innerText = "connected";
}

function disconnected() {
    connectBtn.removeAttribute("disabled");
    disconnectBtn.setAttribute("disabled", true);
    connectionStatus.setAttribute("variant", "negative");
    connectionStatus.innerText = "disconnected";
}

connectBtn.addEventListener("click", () => {
    tone.initialize();

    connectBtn.setAttribute("disabled", true);

    checkNotificationPermission().
        then(() => ble.readStatus()).
        then(() => {
            disconnectBtn.removeAttribute("disabled");
        }).
        catch(err => {
            connectBtn.removeAttribute("disabled");
            console.error(`Something went wrong. ${err}`);
        });
});

disconnectBtn.addEventListener("click", () => {
    ble.disconnect();
    connectBtn.removeAttribute("disabled");
    disconnectBtn.setAttribute("disabled", true);
});

// status
statusSelect.addEventListener("change", event => {
    tone.initialize();

    const status = event.target.value;
    if (status == null || status.length == 0) {
        console.error("invalid status:", status);
        return;
    }
    checkNotificationPermission().
        then(() => ble.writeStatus(status)).
        catch(err => {
            console.error(`Something went wrong. ${err}`);
        });
});

settingGainInput.value = gain.value;
settingGainInput.addEventListener("change", () => {
    console.log(`gain: ${settingGainInput.value}`);
    gain.value = settingGainInput.value;
});

const messageSettings = document.getElementById("message-settings");
callingDialog.addEventListener("confirm", () => {
    callingDialog.open = false;
    ble.writeResponse(messageSettings.response_ok).catch(err => {
        console.error(`Something went wrong. ${err}`);
    });
});
callingDialog.addEventListener("secondary", () => {
    callingDialog.open = false;
    ble.writeResponse(messageSettings.response_busy).catch(err => {
        console.error(`Something went wrong. ${err}`);
    });
});
yoDialog.addEventListener("confirm", () => {
    yoDialog.open = false;
    ble.writeResponse(messageSettings.response_confirm).catch(err => {
        console.error(`Something went wrong. ${err}`);
    });
});

async function checkNotificationPermission() {
    if (window.Notification == null) {
        console.error("Notification is not supported");
        return;
    }

    if (Notification.permission === "granted") {
        return;
    }

    const permission = await Notification.requestPermission();
    if (permission !== "granted") {
        console.error("Notification is not permitted");
    }
}

if (navigator.serviceWorker == null) {
    console.error("ServiceWorker is not supported");
} else {
    navigator.serviceWorker.register("/sw.js", {
        scope: "/",
    }).then(registration => {
        if (registration.installing) {
            console.log("Service worker installing");
        } else if (registration.waiting) {
            console.log("Service worker installed");
        } else if (registration.active) {
            console.log("Service worker active");
        }
    }).catch(err => {
        console.error(`ServiceWorker registration failed with ${err}`);
    });

    navigator.serviceWorker.addEventListener("message", event => {
        console.log(`message: action=${event.data.action}`);
        let resp;
        switch (event.data.action) {
            case "ok":
                resp = messageSettings.response_ok;
                callingDialog.open = false;
                break;
            case "ng":
                resp = messageSettings.response_busy;
                callingDialog.open = false;
                break;
            case "yo_ok":
                resp = messageSettings.response_confirm;
                yoDialog.open = false;
                break;
            default:
                console.error(`unhandled action: ${event.data.action}`);
                return;
        }
        ble.writeResponse(resp).catch(err => {
            console.error(`Something went wrong. ${err}`);
        });
    });
}

async function notify(message, options = {}) {
    tone.play();

    switch (options.tag) {
        case "call":
            Overlay.open(
                callingDialog,
                "modal",
                callingDialog,
                { placement: "none" }
            );
            break;
        case "yo":
            Overlay.open(
                yoDialog,
                "modal",
                yoDialog,
                { placement: "none" }
            );
    }

    const title = "RoomStatusMonitor";
    if (window.Notification != null && Notification.permission === "granted" && navigator.serviceWorker != null) {
        const defaultOptions = {
            lang: "ja",
            // badge: "",
            body: message,
            // icon: "",
            // image: "",
            renotify: true,
            requireInteraction: true,
        };
        const registration = await navigator.serviceWorker.ready;
        return registration.active.postMessage({
            title: title,
            options: { ...defaultOptions, ...options },
        });
    }

    // fallback
    const ok = confirm(`${title}: ${message}`);
    const resp = ok ? messageSettings.response_ok : messageSettings.response_busy;
    return ble.writeResponse(resp);
}
