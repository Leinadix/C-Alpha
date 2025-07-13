"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.deactivate = exports.activate = void 0;
const path = require("path");
const vscode_1 = require("vscode");
const node_1 = require("vscode-languageclient/node");
let client;
function activate(context) {
    // Get the LSP server executable path
    const serverExecutable = context.asAbsolutePath(path.join('bin', process.platform === 'win32' ? 'calpha-lsp.exe' : 'calpha-lsp'));
    // Server options - running the executable
    const serverOptions = {
        run: {
            command: serverExecutable,
            transport: node_1.TransportKind.stdio
        },
        debug: {
            command: serverExecutable,
            transport: node_1.TransportKind.stdio
        }
    };
    // Client options - define which files we handle
    const clientOptions = {
        documentSelector: [
            { scheme: 'file', language: 'calpha' }
        ],
        synchronize: {
            fileEvents: vscode_1.workspace.createFileSystemWatcher('**/*.{calpha,alpha}')
        }
    };
    // Create and start the client
    client = new node_1.LanguageClient('calpha-language-server', 'C-Alpha Language Server', serverOptions, clientOptions);
    // Start the client and launch the server
    client.start();
}
exports.activate = activate;
function deactivate() {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
exports.deactivate = deactivate;
//# sourceMappingURL=extension.js.map