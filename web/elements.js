import moment from "moment";
import { html, LitElement } from 'lit';

class History extends LitElement {
    static properties = {
        histories: { type: Array },
    };

    constructor() {
        super();
        this.histories = [];
    }

    render() {
        let histories = this.histories;
        if (histories.length === 0) {
            histories = histories.concat([{ time: "-", title: "no history", response: "-" }]);
        }
        const row = history => html`<sp-table-row>
            <sp-table-cell>${history.time}</sp-table-cell>
            <sp-table-cell>${history.title}</sp-table-cell>
            <sp-table-cell>${history.response}</sp-table-cell>
        </sp-table-row>`;
        return html`<sp-table size="m">
            <sp-table-head>
                <sp-table-head-cell>Time</sp-table-head-cell>
                <sp-table-head-cell>Title</sp-table-head-cell>
                <sp-table-head-cell>Response</sp-table-head-cell>
            </sp-table-head>
            <sp-table-body>
                ${histories.map(row)}
            </sp-table-body>
        </sp-table>`;
    }

    addHistory({ time, title }) {
        const oldVal = this.histories;
        const history = {
            time: moment(time).format(moment.DATETIME_LOCAL_SECONDS),
            title
        };
        this.histories = [history].concat(this.histories);
        this.requestUpdate("histories", oldVal);
    }
    updateHistoryResponse({ response }) {
        const idx = 0;
        const oldVal = this.histories[idx];
        this.histories[idx].response = response;
        this.requestUpdate(`histories[${idx}]`, oldVal);
    }
}
customElements.define("app-history", History);

class Settings extends LitElement {
    static properties = {
        response_ok: { type: String },
        response_busy: { type: String },
        response_confirm: { type: String },
    };

    constructor() {
        super();
        this.response_ok = "対応可能です！少々お待ちください";
        this.response_busy = "取り込み中です… 要件をお願いします";
        this.response_confirm = "確認しました";
    }

    onchange_message_ok(event) {
        const oldVal = this.response_ok;
        this.response_ok = event.target.value;
        this.requestUpdate("response_ok", oldVal);
    }
    onchange_message_busy(event) {
        const oldVal = this.response_busy;
        this.response_busy = event.target.value;
        this.requestUpdate("response_busy", oldVal);
    }
    onchange_message_confirm(event) {
        const oldVal = this.response_confirm;
        this.response_confirm = event.target.value;
        this.requestUpdate("response_confirm", oldVal);
    }

    render() {
        return html`<overlay-trigger type="modal" placement="none">
            <sp-button slot="trigger" variant="secondary" size="s">Message Settings</sp-button>
            <sp-tray slot="click-content">
                <sp-dialog size="s" dismissable>
                    <h2 slot="heading">Message Settings</h2>
                    <sp-field-label for="ok-message">OK レスポンス</sp-field-label>
                    <sp-textfield id="ok-message" value="${this.response_ok}" @input="${this.onchange_message_ok}"></sp-textfield>
                    <sp-field-label for="busy-message">Busy レスポンス</sp-field-label>
                    <sp-textfield id="busy-message" value="${this.response_busy}" @input="${this.onchange_message_busy}"></sp-textfield>
                    <sp-field-label for="confirm-message">Confirm レスポンス</sp-field-label>
                    <sp-textfield id="confirm-message" value="${this.response_confirm}" @input="${this.onchange_message_confirm}"></sp-textfield>
                </sp-dialog>
            </sp-tray>
        </overlay-trigger>`;
    }
}
customElements.define("app-message-settings", Settings);
