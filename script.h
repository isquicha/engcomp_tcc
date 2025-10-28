#ifndef SCRIPT_H
#define SCRIPT_H

const char JS_CONTENT[] PROGMEM = R"rawliteral(
console.log('[IR Remote] Script carregado');

// SPA Router and State Management
const app = {
    currentPage: 'home',
    remotes: [],
    currentRemote: null,
    currentButton: null,
    recordingInterval: null
};

// Toast Notifications
function showToast(type, title, message) {
    console.log(`[Toast ${type}] ${title}: ${message}`);
    const container = document.getElementById('toast-container');
    const toast = document.createElement('div');
    toast.className = 'toast ' + type;

    const icons = {
        success: '&#10003;',
        error: '&#10005;',
        warning: '&#9888;'
    };

    toast.innerHTML = '<div class="toast-icon">' + (icons[type] || '&#9432;') + '</div>' +
        '<div class="toast-content">' +
        '<div class="toast-title">' + title + '</div>' +
        '<div class="toast-message">' + message + '</div>' +
        '</div>';

    container.appendChild(toast);

    setTimeout(function() {
        toast.style.animation = 'slideInRight 0.3s reverse';
        setTimeout(function() { toast.remove(); }, 300);
    }, 4000);
}

// API Calls with Error Handling
async function apiCall(url, method, body) {
    method = method || 'GET';
    console.log('[API] ' + method + ' ' + url, body);

    try {
        const options = {
            method: method,
            headers: { 'Content-Type': 'application/json' }
        };

        if (body) options.body = JSON.stringify(body);

        const response = await fetch(url, options);
        const data = await response.json();

        console.log('[API Response]', response.status, data);

        if (response.status >= 400) {
            showToast('error', 'Erro', data.error || 'Ocorreu um erro');
            return null;
        }

        return data;
    } catch (error) {
        console.error('[API Error]', error);
        showToast('error', 'Erro de Conexao', 'Nao foi possivel conectar ao servidor');
        return null;
    }
}

// Load Remotes
async function loadRemotes() {
    console.log('[Load] Carregando controles remotos...');
    const data = await apiCall('/api/remotes', 'GET');
    if (data) {
        app.remotes = data.remotes;
        console.log('[Load] Controles carregados:', app.remotes.length);
        if (app.currentPage === 'home') renderHome();
    }
}

// Pages
function renderHome() {
    console.log('[Render] Renderizando home');
    const appDiv = document.getElementById('app');

    let html = '<div class="header">' +
        '<h1>Controle Remoto IR</h1>' +
        '<p>Gerencie seus controles remotos</p>' +
        '</div>' +
        '<div class="card">' +
        '<button class="btn btn-primary btn-block" onclick="showAddRemoteModal()">+ Novo Controle</button>' +
        '</div>' +
        '<div id="remotes-list">';

    if (app.remotes.length === 0) {
        html += '<div class="empty-state">' +
            '<div class="empty-state-icon">&#128190;</div>' +
            '<h3>Nenhum controle cadastrado</h3>' +
            '<p>Clique em "Novo Controle" para comecar</p>' +
            '</div>';
    } else {
        app.remotes.forEach(function(remote) {
            html += '<div class="card">' +
                '<div class="remote-item">' +
                '<div class="remote-name">' + escapeHtml(remote.name) + '</div>' +
                '<div class="remote-actions">' +
                '<button class="btn btn-primary btn-small" onclick="viewRemote(' + remote.id + ')">Usar</button>' +
                '<button class="btn btn-secondary btn-small" onclick="editRemote(' + remote.id + ')">Editar</button>' +
                '<button class="btn btn-danger btn-small" onclick="deleteRemote(' + remote.id + ')">&#10005;</button>' +
                '</div></div></div>';
        });
    }

    html += '</div>';
    appDiv.innerHTML = html;
}

function renderViewRemote() {
    console.log('[Render] Renderizando visualizacao do controle');
    const remote = app.currentRemote;
    const appDiv = document.getElementById('app');

    let html = '<a href="#" class="nav-back" onclick="navigateTo(\'home\'); return false;">&#8592; Voltar</a>' +
        '<div class="header">' +
        '<h1>' + escapeHtml(remote.name) + '</h1>' +
        '<p>Clique nos botoes para enviar sinais IR</p>' +
        '</div><div class="card">';

    if (remote.buttons.length === 0) {
        html += '<div class="empty-state">' +
            '<div class="empty-state-icon">&#128290;</div>' +
            '<p>Nenhum botao cadastrado</p>' +
            '<p>Va em Editar para adicionar botoes</p></div>';
    } else {
        html += '<div class="button-grid">';
        remote.buttons.forEach(function(button) {
            const hasSignal = button.hasSignal;
            html += '<div class="ir-button ' + (hasSignal ? '' : 'no-signal') + '" ' +
                (hasSignal ? 'onclick="sendSignal(' + remote.id + ',' + button.id + ')"' : '') + '>' +
                (hasSignal ? '<span class="badge"></span>' : '') +
                '<div>' + escapeHtml(button.name) + '</div></div>';
        });
        html += '</div>';
    }

    html += '</div>';
    appDiv.innerHTML = html;
}

function renderEditRemote() {
    console.log('[Render] Renderizando edicao do controle');
    const remote = app.currentRemote;
    const appDiv = document.getElementById('app');

    let html = '<a href="#" class="nav-back" onclick="navigateTo(\'home\'); return false;">&#8592; Voltar</a>' +
        '<div class="header"><h1>Editar: ' + escapeHtml(remote.name) + '</h1></div>' +
        '<div class="card">' +
        '<button class="btn btn-primary btn-block" onclick="showAddButtonModal()">+ Novo Botao</button>' +
        '</div><div class="card"><h3 style="margin-bottom: 16px;">Botoes</h3>';

    if (remote.buttons.length === 0) {
        html += '<div class="empty-state"><p>Nenhum botao cadastrado</p></div>';
    } else {
        html += '<div class="button-grid">';
        remote.buttons.forEach(function(button) {
            html += '<div class="ir-button">' +
                (button.hasSignal ? '<span class="badge"></span>' : '') +
                '<div style="margin-bottom: 12px;">' + escapeHtml(button.name) + '</div>' +
                '<div style="display: flex; gap: 8px; justify-content: center;">' +
                '<button class="btn btn-success btn-small" onclick="startRecording(' + remote.id + ',' + button.id + ')">' +
                (button.hasSignal ? '&#8635;' : '&#9679;') + '</button>' +
                '<button class="btn btn-secondary btn-small" onclick="editButton(' + remote.id + ',' + button.id + ')">&#9998;</button>' +
                '<button class="btn btn-danger btn-small" onclick="deleteButton(' + remote.id + ',' + button.id + ')">&#10005;</button>' +
                '</div></div>';
        });
        html += '</div>';
    }

    html += '</div>';
    appDiv.innerHTML = html;
}

// Navigation
function navigateTo(page, data) {
    console.log('[Nav] Navegando para:', page);
    app.currentPage = page;

    if (page === 'home') {
        loadRemotes();
    } else if (page === 'view') {
        app.currentRemote = data;
        renderViewRemote();
    } else if (page === 'edit') {
        app.currentRemote = data;
        renderEditRemote();
    }
}

// Actions
async function viewRemote(id) {
    const remote = app.remotes.find(function(r) { return r.id === id; });
    if (remote) navigateTo('view', remote);
}

async function editRemote(id) {
    const remote = app.remotes.find(function(r) { return r.id === id; });
    if (remote) navigateTo('edit', remote);
}

async function deleteRemote(id) {
    if (!confirm('Deseja realmente deletar este controle?')) return;

    const result = await apiCall('/api/remote/delete', 'POST', { remoteId: id });
    if (result) {
        showToast('success', 'Sucesso', 'Controle deletado');
        loadRemotes();
    }
}

async function deleteButton(remoteId, buttonId) {
    if (!confirm('Deseja realmente deletar este botao?')) return;

    const result = await apiCall('/api/button/delete', 'POST', { remoteId: remoteId, buttonId: buttonId });
    if (result) {
        showToast('success', 'Sucesso', 'Botao deletado');
        await loadRemotes();
        const remote = app.remotes.find(function(r) { return r.id === remoteId; });
        if (remote) navigateTo('edit', remote);
    }
}

async function sendSignal(remoteId, buttonId) {
    const result = await apiCall('/api/signal/send', 'POST', { remoteId: remoteId, buttonId: buttonId });
    if (result) {
        showToast('success', 'Enviado', 'Sinal IR transmitido');
    }
}

async function startRecording(remoteId, buttonId) {
    const result = await apiCall('/api/record/start', 'POST', { remoteId: remoteId, buttonId: buttonId });
    if (result) {
        showRecordingModal(remoteId, buttonId);
    }
}

async function stopRecording() {
    console.log('[Record] Parando gravacao');
    await apiCall('/api/record/stop', 'POST');
    if (app.recordingInterval) {
        clearInterval(app.recordingInterval);
        app.recordingInterval = null;
    }
    closeModal('recording-modal');
    await loadRemotes();
    const remote = app.remotes.find(function(r) { return r.id === app.currentRemote.id; });
    if (remote) navigateTo('edit', remote);
}

// Modals
function showAddRemoteModal() {
    console.log('[Modal] Abrindo modal novo controle');
    showModal(
        '<div class="modal-header"><h2>Novo Controle</h2></div>' +
        '<div class="input-group">' +
        '<label>Nome do Controle</label>' +
        '<input type="text" id="remote-name" class="input-field" placeholder="Ex: TV Sala">' +
        '</div>' +
        '<div class="modal-footer">' +
        '<button class="btn btn-secondary" onclick="closeModal()">Cancelar</button>' +
        '<button class="btn btn-primary" onclick="addRemote()">Adicionar</button>' +
        '</div>'
    );
    setTimeout(function() {
        const input = document.getElementById('remote-name');
        if (input) input.focus();
    }, 100);
}

function showAddButtonModal() {
    console.log('[Modal] Abrindo modal novo botao');
    showModal(
        '<div class="modal-header"><h2>Novo Botao</h2></div>' +
        '<div class="input-group">' +
        '<label>Nome do Botao</label>' +
        '<input type="text" id="button-name" class="input-field" placeholder="Ex: Power">' +
        '</div>' +
        '<div class="modal-footer">' +
        '<button class="btn btn-secondary" onclick="closeModal()">Cancelar</button>' +
        '<button class="btn btn-primary" onclick="addButton()">Adicionar</button>' +
        '</div>'
    );
    setTimeout(function() {
        const input = document.getElementById('button-name');
        if (input) input.focus();
    }, 100);
}

function editButton(remoteId, buttonId) {
    const remote = app.remotes.find(function(r) { return r.id === remoteId; });
    const button = remote.buttons.find(function(b) { return b.id === buttonId; });

    showModal(
        '<div class="modal-header"><h2>Editar Botao</h2></div>' +
        '<div class="input-group">' +
        '<label>Nome do Botao</label>' +
        '<input type="text" id="button-name" class="input-field" value="' + escapeHtml(button.name) + '">' +
        '</div>' +
        '<div class="modal-footer">' +
        '<button class="btn btn-secondary" onclick="closeModal()">Cancelar</button>' +
        '<button class="btn btn-primary" onclick="saveButtonEdit(' + remoteId + ',' + buttonId + ')">Salvar</button>' +
        '</div>'
    );
}

function showRecordingModal(remoteId, buttonId) {
    console.log('[Modal] Abrindo modal de gravacao');
    showModal(
        '<div class="recording-modal">' +
        '<div class="modal-header"><h2>Gravando Sinal IR</h2></div>' +
        '<div class="pulse-ring"></div>' +
        '<p>Aponte o controle remoto para o receptor</p>' +
        '<p>e pressione o botao desejado</p>' +
        '<div class="modal-footer">' +
        '<button class="btn btn-danger btn-block" onclick="stopRecording()">Cancelar</button>' +
        '</div></div>',
        'recording-modal'
    );

    app.recordingInterval = setInterval(async function() {
        await loadRemotes();
        const remote = app.remotes.find(function(r) { return r.id === remoteId; });
        if (remote) {
            const button = remote.buttons.find(function(b) { return b.id === buttonId; });
            if (button && button.hasSignal) {
                showToast('success', 'Gravado!', 'Sinal IR capturado com sucesso');
                stopRecording();
            }
        }
    }, 1000);
}

function showModal(content, id) {
    id = id || 'modal';
    console.log('[Modal] Abrindo modal:', id);

    let modal = document.getElementById(id);
    if (!modal) {
        modal = document.createElement('div');
        modal.id = id;
        modal.className = 'modal';
        modal.innerHTML = '<div class="modal-content">' + content + '</div>';
        document.body.appendChild(modal);

        modal.addEventListener('click', function(e) {
            if (e.target === modal) closeModal(id);
        });
    } else {
        modal.querySelector('.modal-content').innerHTML = content;
    }

    setTimeout(function() { modal.classList.add('active'); }, 10);
}

function closeModal(id) {
    id = id || 'modal';
    console.log('[Modal] Fechando modal:', id);
    const modal = document.getElementById(id);
    if (modal) {
        modal.classList.remove('active');
        setTimeout(function() { modal.remove(); }, 300);
    }
}

// Form Actions
async function addRemote() {
    const name = document.getElementById('remote-name').value.trim();
    if (!name) {
        showToast('warning', 'Atencao', 'Digite um nome para o controle');
        return;
    }

    const result = await apiCall('/api/remote/add', 'POST', { name: name });
    if (result) {
        showToast('success', 'Sucesso', 'Controle adicionado');
        closeModal();
        loadRemotes();
    }
}

async function addButton() {
    const name = document.getElementById('button-name').value.trim();
    if (!name) {
        showToast('warning', 'Atencao', 'Digite um nome para o botao');
        return;
    }

    const result = await apiCall('/api/button/add', 'POST', {
        remoteId: app.currentRemote.id,
        name: name
    });

    if (result) {
        showToast('success', 'Sucesso', 'Botao adicionado');
        closeModal();
        await loadRemotes();
        const remote = app.remotes.find(function(r) { return r.id === app.currentRemote.id; });
        if (remote) navigateTo('edit', remote);
    }
}

async function saveButtonEdit(remoteId, buttonId) {
    const name = document.getElementById('button-name').value.trim();
    if (!name) {
        showToast('warning', 'Atencao', 'Digite um nome para o botao');
        return;
    }

    const result = await apiCall('/api/button/edit', 'POST', {
        remoteId: remoteId,
        buttonId: buttonId,
        name: name
    });

    if (result) {
        showToast('success', 'Sucesso', 'Botao atualizado');
        closeModal();
        await loadRemotes();
        const remote = app.remotes.find(function(r) { return r.id === remoteId; });
        if (remote) navigateTo('edit', remote);
    }
}

// Utility
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Initialize
document.addEventListener('DOMContentLoaded', function() {
    console.log('[Init] Inicializando aplicacao');
    loadRemotes();
});
)rawliteral";

#endif
