#ifndef STYLES_H
#define STYLES_H

const char CSS_CONTENT[] PROGMEM = R"rawliteral(
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

:root {
    --primary: #4f46e5;
    --primary-dark: #4338ca;
    --secondary: #64748b;
    --success: #10b981;
    --danger: #ef4444;
    --warning: #f59e0b;
    --bg: #0f172a;
    --surface: #1e293b;
    --surface-light: #334155;
    --text: #f1f5f9;
    --text-secondary: #94a3b8;
    --border: #334155;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    background: var(--bg);
    color: var(--text);
    line-height: 1.6;
    min-height: 100vh;
}

#app {
    max-width: 800px;
    margin: 0 auto;
    padding: 20px;
    animation: fadeIn 0.3s;
}

@keyframes fadeIn {
    from { opacity: 0; transform: translateY(10px); }
    to { opacity: 1; transform: translateY(0); }
}

.header {
    text-align: center;
    margin-bottom: 40px;
    padding: 30px 0;
}

.header h1 {
    font-size: 2.5rem;
    background: linear-gradient(135deg, var(--primary), #818cf8);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    margin-bottom: 10px;
}

.header p {
    color: var(--text-secondary);
    font-size: 1.1rem;
}

.nav-back {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    color: var(--text-secondary);
    text-decoration: none;
    padding: 10px 16px;
    border-radius: 8px;
    transition: all 0.2s;
    margin-bottom: 20px;
    font-weight: 500;
}

.nav-back:hover {
    background: var(--surface);
    color: var(--text);
}

.card {
    background: var(--surface);
    border-radius: 16px;
    padding: 24px;
    margin-bottom: 20px;
    border: 1px solid var(--border);
    transition: all 0.3s;
}

.card:hover {
    border-color: var(--primary);
    box-shadow: 0 8px 24px rgba(79, 70, 229, 0.15);
}

.remote-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 20px;
    background: var(--surface-light);
    border-radius: 12px;
    margin-bottom: 12px;
    transition: all 0.2s;
}

.remote-item:hover {
    background: var(--primary);
    transform: translateX(4px);
}

.remote-name {
    font-size: 1.2rem;
    font-weight: 600;
}

.remote-actions {
    display: flex;
    gap: 8px;
}

.btn {
    padding: 12px 24px;
    border: none;
    border-radius: 10px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
    font-size: 0.95rem;
    display: inline-flex;
    align-items: center;
    gap: 8px;
}

.btn:active {
    transform: scale(0.98);
}

.btn-primary {
    background: var(--primary);
    color: white;
}

.btn-primary:hover {
    background: var(--primary-dark);
    box-shadow: 0 4px 12px rgba(79, 70, 229, 0.4);
}

.btn-secondary {
    background: var(--surface-light);
    color: var(--text);
}

.btn-secondary:hover {
    background: var(--secondary);
}

.btn-success {
    background: var(--success);
    color: white;
}

.btn-success:hover {
    background: #059669;
}

.btn-danger {
    background: var(--danger);
    color: white;
}

.btn-danger:hover {
    background: #dc2626;
}

.btn-small {
    padding: 8px 16px;
    font-size: 0.875rem;
}

.btn-block {
    width: 100%;
    justify-content: center;
}

.button-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
    gap: 12px;
    margin-top: 20px;
}

.ir-button {
    padding: 20px;
    background: var(--surface-light);
    border: 2px solid var(--border);
    border-radius: 12px;
    cursor: pointer;
    transition: all 0.2s;
    text-align: center;
    font-weight: 600;
    position: relative;
    min-height: 80px;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}

.ir-button:hover {
    background: var(--primary);
    border-color: var(--primary);
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(79, 70, 229, 0.3);
}

.ir-button:active {
    transform: translateY(0);
}

.ir-button.no-signal {
    opacity: 0.5;
    cursor: not-allowed;
}

.ir-button.no-signal:hover {
    background: var(--surface-light);
    border-color: var(--border);
    transform: none;
}

.ir-button .badge {
    position: absolute;
    top: 8px;
    right: 8px;
    width: 8px;
    height: 8px;
    background: var(--success);
    border-radius: 50%;
}

.input-group {
    margin-bottom: 20px;
}

.input-group label {
    display: block;
    margin-bottom: 8px;
    color: var(--text-secondary);
    font-weight: 500;
}

.input-field {
    width: 100%;
    padding: 14px 16px;
    background: var(--surface-light);
    border: 2px solid var(--border);
    border-radius: 10px;
    color: var(--text);
    font-size: 1rem;
    transition: all 0.2s;
}

.input-field:focus {
    outline: none;
    border-color: var(--primary);
    background: var(--surface);
}

.modal {
    display: none;
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: rgba(0, 0, 0, 0.8);
    z-index: 1000;
    align-items: center;
    justify-content: center;
    animation: fadeIn 0.2s;
}

.modal.active {
    display: flex;
}

.modal-content {
    background: var(--surface);
    border-radius: 16px;
    padding: 32px;
    max-width: 500px;
    width: 90%;
    border: 1px solid var(--border);
    animation: slideUp 0.3s;
}

@keyframes slideUp {
    from { opacity: 0; transform: translateY(20px); }
    to { opacity: 1; transform: translateY(0); }
}

.modal-header {
    margin-bottom: 24px;
}

.modal-header h2 {
    font-size: 1.5rem;
    margin-bottom: 8px;
}

.modal-footer {
    display: flex;
    gap: 12px;
    margin-top: 24px;
}

.recording-modal {
    text-align: center;
}

.pulse-ring {
    width: 100px;
    height: 100px;
    border-radius: 50%;
    background: var(--danger);
    margin: 20px auto;
    animation: pulse 1.5s ease-in-out infinite;
}

@keyframes pulse {
    0%, 100% { transform: scale(1); opacity: 1; }
    50% { transform: scale(1.1); opacity: 0.7; }
}

#toast-container {
    position: fixed;
    top: 20px;
    right: 20px;
    z-index: 2000;
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.toast {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 12px;
    padding: 16px 20px;
    min-width: 300px;
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.3);
    display: flex;
    align-items: center;
    gap: 12px;
    animation: slideInRight 0.3s;
}

@keyframes slideInRight {
    from { transform: translateX(100%); opacity: 0; }
    to { transform: translateX(0); opacity: 1; }
}

.toast.success {
    border-left: 4px solid var(--success);
}

.toast.error {
    border-left: 4px solid var(--danger);
}

.toast.warning {
    border-left: 4px solid var(--warning);
}

.toast-icon {
    font-size: 1.5rem;
}

.toast-content {
    flex: 1;
}

.toast-title {
    font-weight: 600;
    margin-bottom: 4px;
}

.toast-message {
    font-size: 0.875rem;
    color: var(--text-secondary);
}

.empty-state {
    text-align: center;
    padding: 60px 20px;
    color: var(--text-secondary);
}

.empty-state-icon {
    font-size: 4rem;
    margin-bottom: 16px;
    opacity: 0.3;
}

.loading {
    text-align: center;
    padding: 40px;
    color: var(--text-secondary);
}

.spinner {
    width: 40px;
    height: 40px;
    border: 4px solid var(--border);
    border-top-color: var(--primary);
    border-radius: 50%;
    animation: spin 1s linear infinite;
    margin: 0 auto 16px;
}

@keyframes spin {
    to { transform: rotate(360deg); }
}

@media (max-width: 768px) {
    .header h1 {
        font-size: 2rem;
    }

    .button-grid {
        grid-template-columns: repeat(2, 1fr);
    }

    #toast-container {
        left: 20px;
        right: 20px;
    }

    .toast {
        min-width: auto;
    }

    .remote-actions {
        flex-direction: column;
    }
}
)rawliteral";

#endif
