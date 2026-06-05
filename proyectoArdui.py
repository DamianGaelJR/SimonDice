import serial
import csv
import os
import time
import threading
from queue import Queue
import requests
import gspread
from oauth2client.service_account import ServiceAccountCredentials

# ========================================================
# CONFIGURACIÓN DE PUERTOS Y SERVICIOS NUBE
# ========================================================
PORT_SERIE = 'COM4'
BAUD_RATE = 115200
ARCHIVO_CSV = 'simon_dice_dataset.csv'

# Configuración ThingSpeak
THINGSPEAK_API_KEY = "LT10P03XWY1C5G39"

# Configuración Google Sheets
NOMBRE_DOCUMENTO_GOOGLE = "Datos_Simon_Dice"
ARCHIVO_CREDENCIALES = "credenciales.json"
# ========================================================

# Cola de datos para comunicación entre hilos (Evita bloqueos del puerto serie)
cola_nube = Queue()

# Encabezados de las columnas
headers = [
    "timestamp_ms", "distancia_cm", "movimiento", "color_detectado",
    "r", "g", "b", "tiempo_reaccion_ms", "ronda", "acierto", 
    "velocidad_ms", "longitud_secuencia"
]

# Inicializar archivo local si no existe
if not os.path.exists(ARCHIVO_CSV):
    with open(ARCHIVO_CSV, mode='w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(headers)

# Global para mantener la referencia a la hoja
hoja_google = None

def conectar_google_sheets():
    """Función interna para intentar establecer o recuperar la conexión con la API"""
    global hoja_google
    try:
        scope = [
            'https://www.googleapis.com/auth/spreadsheets',
            'https://www.googleapis.com/auth/drive'
        ]
        credenciales = ServiceAccountCredentials.from_json_keyfile_name(ARCHIVO_CREDENCIALES, scope)
        cliente_google = gspread.authorize(credenciales)
        # Intenta abrir el documento y toma la primera pestaña disponible
        hoja_google = cliente_google.open(NOMBRE_DOCUMENTO_GOOGLE).sheet1
        print("[Nube] ¡Conexión exitosa con Google Sheets!")
        return True
    except Exception as e:
        print(f"[Nube Aviso]: No se pudo conectar a Google Sheets. Revisa la API de Drive o credenciales.json. Detalle: {e}")
        hoja_google = None
        return False

# ========================================================
# TRABAJADOR EN SEGUNDO PLANO (HILO PARA LA NUBE)
# ========================================================
def trabajador_nube():
    """Este hilo se encarga exclusivamente de enviar datos a internet
    sin interferir con la velocidad de lectura del puerto USB."""
    global hoja_google
    
    # Intento de conexión inicial
    conectar_google_sheets()

    while True:
        # Extraer el registro de la cola de espera
        valores = cola_nube.get()
        if valores is None:
            break
            
        # 1. Envío asíncrono a Google Sheets (con auto-reconexión si estaba caída)
        if hoja_google is None:
            # Si no estaba conectado, intenta reconectar justo antes de enviar el dato
            conectar_google_sheets()
            
        if hoja_google is not None:
            try:
                hoja_google.append_row(valores)
            except Exception as e:
                print(f"[Error Google Sheets]: No se pudo insertar la fila en la nube: {e}")
                # Si el token expiró, forzamos reconexión para el siguiente dato
                if "auth" in str(e).lower() or "token" in str(e).lower():
                    hoja_google = None

        # 2. Envío asíncrono a ThingSpeak (Mapeo de métricas principales)
        try:
            url_ts = f"https://api.thingspeak.com/update?api_key={THINGSPEAK_API_KEY}"
            payload = {
                "field1": valores[1],  # distancia_cm
                "field2": valores[7],  # tiempo_reaccion_ms
                "field3": valores[8],  # ronda
                "field4": valores[9]   # acierto
            }
            # Timeout corto de 4 segundos para evitar que se sature el hilo si ThingSpeak responde lento
            requests.get(url_ts, params=payload, timeout=4)
        except Exception as e:
            print(f"[Error ThingSpeak]: {e}")
            pass
            
        cola_nube.task_done()

# Arrancar el hilo de la nube de forma paralela
hilo_internet = threading.Thread(target=trabajador_nube, daemon=True)
hilo_internet.start()

# ========================================================
# BUCLE PRINCIPAL (LECTURA HARDWARE)
# ========================================================
try:
    ser = serial.Serial(PORT_SERIE, BAUD_RATE, timeout=1)
    time.sleep(2)  # Espera para que el Arduino complete el auto-reinicio por USB
    ser.flushInput()
    
    print(f"\n--- Sistema Híbrido Optimizado Listo ---")
    print(f"1. Escribiendo localmente en: {ARCHIVO_CSV}")
    print(f"2. Transmitiendo en tiempo real a Google Sheets y ThingSpeak")
    print("Mueve la mano frente a los sensores para iniciar el juego...")
    
    contador_registros = 0

    while True:
        if ser.in_waiting > 0:
            linea = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if not linea:
                continue
            if "===" in linea:
                print(f"\n[Arduino Informa]: {linea}")
                continue
            if "timestamp_ms" in linea:
                continue
            
            valores = linea.split(',')
            
            if len(valores) == 12:
                # Guardar inmediatamente en el disco duro local (Prioridad cero pérdidas)
                with open(ARCHIVO_CSV, mode='a', newline='', encoding='utf-8') as f:
                    writer = csv.writer(f)
                    writer.writerow(valores)
                
                contador_registros += 1
                print(f"[REGISTRO #{contador_registros}] Guardado local OK. Enviando a la nube...")
                
                # Mandar una copia de la fila a la cola para procesamiento en segundo plano
                cola_nube.put(valores)
            else:
                if len(linea) > 0:
                    print(f"[Texto Omitido/Ruido]: {linea}")

except serial.SerialException as e:
    print(f"\n[ERROR CRÍTICO]: Puerto {PORT_SERIE} desconectado o en uso. {e}")
except KeyboardInterrupt:
    print("\nAdquisición finalizada manualmente por el usuario (Ctrl+C).")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
    print("Proceso terminado de manera segura.")