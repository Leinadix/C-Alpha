import * as path from 'path';
import { workspace, ExtensionContext } from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
    // Get the LSP server executable path
    const serverExecutable = context.asAbsolutePath(
        path.join('bin', process.platform === 'win32' ? 'calpha-lsp.exe' : 'calpha-lsp')
    );

    // Server options - running the executable
    const serverOptions: ServerOptions = {
        run: {
            command: serverExecutable,
            transport: TransportKind.stdio
        },
        debug: {
            command: serverExecutable,
            transport: TransportKind.stdio
        }
    };

    // Client options - define which files we handle
    const clientOptions: LanguageClientOptions = {
        documentSelector: [
            { scheme: 'file', language: 'calpha' }
        ],
        synchronize: {
            fileEvents: workspace.createFileSystemWatcher('**/*.{calpha,alpha}')
        }
    };

    // Create and start the client
    client = new LanguageClient(
        'calpha-language-server',
        'C-Alpha Language Server',
        serverOptions,
        clientOptions
    );

    // Start the client and launch the server
    client.start();
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
} 