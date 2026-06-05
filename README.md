# SimonDice (IoT)

Proyecto: **Simon Dice** con Arduino (TCS3200) y procesamiento en Python.

## Estructura
- `ProyectoArdui.ino`: Lógica del juego en el microcontrolador.
- `proyectoArdui.py`: Lee datos del puerto serie, guarda CSV local y envía a Google Sheets + ThingSpeak.
- `simon_dice_dataset.csv`: Dataset local (puede no subirse a GitHub por seguridad/volumen).
- `credenciales.json`: Credenciales de Google (NO se suben a GitHub).

## Requisitos
### Arduino
- Placa compatible con Arduino (y librerías estándar).
- Conexión de sensores/pines según el sketch.

### Python
- Python 3.x
- Dependencias (instalar en tu entorno):
  - `pyserial`
  - `gspread`
  - `oauth2client`
  - `requests`

## Configuración de `credenciales.json`
1. Crea un Service Account en Google Cloud.
2. Descarga el archivo JSON.
3. Renómbralo o ponlo como `credenciales.json` en la raíz del proyecto.

## Ejecución
1. Abre el IDE de Arduino y sube `ProyectoArdui.ino`.
2. Ejecuta `proyectoArdui.py`.
3. Ajusta `PORT_SERIE` en `proyectoArdui.py` si tu COM es distinto.

## Publicación / GitHub
- `credenciales.json` está excluido con `.gitignore`.
- Ajusta también el tratamiento de `simon_dice_dataset.csv` según tu preferencia.

