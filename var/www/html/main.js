let totalDefs = 2;

  function esc(s) {
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
  }
  function setStatus(type, msg) {
    const el = document.getElementById('status-line');
    el.className = 'status-line ' + type;
    el.textContent = msg;
  }
  function today() { return new Date().toISOString().slice(0,10); }

  async function submitDefinition() {
    const handleEl = document.getElementById('handle');
    const defEl    = document.getElementById('definition');
    const btn      = document.getElementById('submit-btn');
    const btnText  = document.getElementById('btn-text');
    const aiBox    = document.getElementById('ai-box');
    const aiText   = document.getElementById('ai-text');

    const handle     = handleEl.value.trim() || 'anonymous';
    const definition = defEl.value.trim();

    if (!definition || definition.length < 15) {
      setStatus('err', '[ ERROR ] Write at least 15 characters.');
      return;
    }

    btn.disabled = true;
    btnText.innerHTML = '<span class="spinner"></span> processing...';
    setStatus('', '');
    aiBox.className = 'ai-review-box';

    try {
      const res = await fetch('https://api.anthropic.com/v1/messages', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          model: 'claude-sonnet-4-20250514',
          max_tokens: 1000,
          system: `You are a senior systems programmer reviewing community-submitted definitions of "webserv" — a C++98 HTTP/1.1 server built from scratch at 42 School, modeled after nginx. It uses poll() for I/O multiplexing, has no external networking libraries, and implements config parsing, static file serving, and CGI.

Write a 2–3 sentence technical review of the submitted definition. Be precise, collegial, and constructive. Confirm what's accurate. If something is incomplete or slightly off, gently note it. Plain text only, no markdown, no lists.`,
          messages: [{ role: 'user', content: `Handle: ${handle}\n\nDefinition: "${definition}"` }]
        })
      });

      const data   = await res.json();
      const review = data?.content?.[0]?.text || null;

      appendDef(handle, definition);
      defEl.value = '';
      handleEl.value = '';
      setStatus('ok', `[ OK ] Definition #${totalDefs} added.`);

      if (review) {
        aiText.textContent = review;
        aiBox.className = 'ai-review-box visible';
      }
    } catch(err) {
      appendDef(handle, definition);
      defEl.value = '';
      handleEl.value = '';
      setStatus('ok', '[ OK ] Definition added. (AI review unavailable)');
    }

    btn.disabled = false;
    btnText.textContent = '[ SUBMIT ]';
  }

  function appendDef(handle, text) {
    const list  = document.getElementById('defs-list');
    const entry = document.createElement('div');
    entry.className = 'community-def';
    entry.innerHTML = `
      <div class="community-def-header">
        <span class="def-handle">${esc(handle)}</span>
        <span class="def-ts">${today()}</span>
      </div>
      <p>${esc(text)}</p>`;
    list.appendChild(entry);
    totalDefs++;
  }

  /* ── nav smooth scroll ── */
  document.querySelectorAll('.nav-bar a').forEach(a => {
    a.addEventListener('click', e => {
      e.preventDefault();
      document.querySelectorAll('.nav-bar a').forEach(x => x.classList.remove('active'));
      a.classList.add('active');
      const target = document.querySelector(a.getAttribute('href'));
      if (target) target.scrollIntoView({ behavior: 'smooth', block: 'start' });
    });
  });

  /* ── POST form — file handling ── */
  function handleFileSelect(input) {
    const file = input.files[0];
    if (!file) return;
    document.getElementById('fc-name').textContent    = file.name;
    document.getElementById('fc-size').textContent    = fmtBytes(file.size);
    document.getElementById('file-chosen').classList.add('visible');
    // debug panel
    document.getElementById('prev-filename').textContent = file.name;
    document.getElementById('prev-mime').textContent     = file.type || 'application/octet-stream';
    document.getElementById('prev-size').textContent     = fmtBytes(file.size);
  }

  function clearFile() {
    document.getElementById('pf-file').value           = '';
    document.getElementById('file-chosen').classList.remove('visible');
    document.getElementById('prev-filename').textContent = '…';
    document.getElementById('prev-mime').textContent     = 'application/octet-stream';
    document.getElementById('prev-size').textContent     = '(no file)';
  }

  function fmtBytes(n) {
    if (n < 1024)    return n + ' B';
    if (n < 1048576) return (n/1024).toFixed(1) + ' KB';
    return (n/1048576).toFixed(1) + ' MB';
  }

  /* ── drag & drop ── */
  const zone = document.getElementById('file-zone');
  zone.addEventListener('dragover',  e => { e.preventDefault(); zone.classList.add('over'); });
  zone.addEventListener('dragleave', () => zone.classList.remove('over'));
  zone.addEventListener('drop', e => {
    e.preventDefault();
    zone.classList.remove('over');
    if (e.dataTransfer.files.length) {
      document.getElementById('pf-file').files = e.dataTransfer.files;
      handleFileSelect(document.getElementById('pf-file'));
    }
  });

  /* ── live debug preview ── */
  function livePreview() {
    const t = document.getElementById('pf-title').value;
    const c = document.getElementById('pf-content').value;
    document.getElementById('prev-title').textContent   = t ? clip(t, 60) : '(empty)';
    document.getElementById('prev-content').textContent = c ? clip(c.replace(/\n/g,'↵'), 70) : '(empty)';
  }
  function clip(s, n) { return s.length > n ? s.slice(0,n) + '…' : s; }

  /* ── basic client-side validation ── */
  document.getElementById('upload-form').addEventListener('submit', function(e) {
    const title   = document.getElementById('pf-title').value.trim();
    const content = document.getElementById('pf-content').value.trim();
    if (!title || !content) {
      e.preventDefault();
      alert('[ ERROR ] Title and Content are required before sending the request.');
    }
  });