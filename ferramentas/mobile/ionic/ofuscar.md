Sim, dá para usar o **javascript-obfuscator** antes de gerar o APK no Ionic/Nx, mas ele não está integrado nativamente ao build.
Você precisa adicioná-lo como uma **etapa pós-build**, antes do `capacitor copy`/`sync`.

### 🔹 Como fazer

1. **Instalar a lib**

```sh
npm install --save-dev javascript-obfuscator
```

2. **Criar um script Node para ofuscar o código**
   Crie um arquivo, por exemplo `scripts/obfuscate.js`:

```js
const JavaScriptObfuscator = require('javascript-obfuscator');
const fs = require('fs');
const path = require('path');

const distPath = path.join(__dirname, '../dist/apps/nomeDoApp'); // ajuste para seu app

function walkDir(dir, callback) {
  fs.readdirSync(dir).forEach(file => {
    const fullPath = path.join(dir, file);
    if (fs.statSync(fullPath).isDirectory()) {
      walkDir(fullPath, callback);
    } else if (file.endsWith('.js')) {
      callback(fullPath);
    }
  });
}

walkDir(distPath, (filePath) => {
  const code = fs.readFileSync(filePath, 'utf8');
  const obfuscated = JavaScriptObfuscator.obfuscate(code, {
    compact: true,
    controlFlowFlattening: true,
    deadCodeInjection: true,
    stringArray: true,
    stringArrayEncoding: ['rc4'],
    stringArrayThreshold: 0.75
  });
  fs.writeFileSync(filePath, obfuscated.getObfuscatedCode(), 'utf8');
  console.log(`Ofuscado: ${filePath}`);
});
```

3. **Alterar o script de build no `package.json`**

Por exemplo, se você quiser ofuscar só no build de produção do nomeDoApp:

```json
"scripts": {
  "RELEASE_NOMEDOAPP_AND": "node scripts/obfuscate.js"
}
```

🔹 Isso garante que, **depois de gerar o build Angular**, ele vai rodar o script e ofuscar os `.js` no `dist/` antes de copiar para o Android.

---

### 📌 Outra opção (mais automática)

Você também pode criar um **builder customizado no Angular/Nx** usando `@angular-builders/custom-webpack` e integrar o `javascript-obfuscator` como **plugin do Webpack**, mas é mais trabalhoso.
O método com script Node é mais simples para Ionic.
