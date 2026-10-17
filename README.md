# Low Latency Tweaks (Geode)

Mod para **Geometry Dash 2.2081 (Windows)** con **Geode 5.11.2**.
Si actualizas GD o Geode, cambia `gd` y `geode` en `mod.json` por las versiones nuevas.

## Compilar sin instalar nada (GitHub Actions)
1. Crea un repositorio en github.com y sube TODO el contenido de esta carpeta (incluida `.github`).
2. Pestaña **Actions** → espera a que termine "Build Geode Mod" (unos minutos).
3. Abre la ejecución, baja el artefacto **Build Output** y descomprímelo: ahí está `cesar.lowlatency.geode`.

## Instalar
Copia `cesar.lowlatency.geode` a `<carpeta de Geometry Dash>\geode\mods\` y reinicia el juego.
Ajustes: Geode → Low Latency Tweaks → Settings.

## Compilar en tu PC (opcional)
Geode CLI + Visual Studio Build Tools (carga de trabajo C++). Dentro de esta carpeta: `geode build`.
Guía oficial: docs.geode-sdk.org

## Qué hace
- Desactiva VSync (se aplica al iniciar y al cambiar el ajuste).
- Fuerza un límite de FPS (por defecto 360) sobre cualquier cambio de intervalo del juego.
- Prioridad alta del proceso y sin limitación de CPU por ahorro de energía (EcoQoS).
- Temporizador de Windows de 1 ms.
- Opcional y experimental: `glFinish` tras cada frame.

## Notas
- GD 2.2081 ya trae VSync y "Unlock FPS" nativos (Opciones → Video → Advanced). Con el mod activo, el límite del mod manda.
- Para probar: primero VSync off + 360 FPS; después activa/desactiva "Esperar a la GPU cada frame" y compara.
- El log de Geode muestra una línea `Low Latency: ...` con lo que se aplicó.
