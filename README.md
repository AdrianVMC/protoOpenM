# openMS 🎵
**openMS** es un proyecto en lenguaje C que simula un reproductor de música por línea de comandos, inspirado en Spotify. Está pensado como una práctica educativa sin uso de sockets ni bases de datos, utilizando archivos `.txt` como almacenamiento.

## 📁 Estructura del proyecto
openMS/
├── client/              # Lógica del CLI principal
├── server/              # (Opcional) lógica de simulación del servidor
├── include/             # Archivos .h con definiciones compartidas
├── utils/               # Funciones auxiliares para manipular archivos
├── data/                # "Base de datos" en formato .txt
├── Makefile             # Archivo para compilar el proyecto
└── README.md            # Este documento

## ⚙️ Compilación
Asegúrate de tener `gcc` y `make` instalados. Luego, en la raíz del proyecto, ejecuta:

make

Esto generará un archivo ejecutable llamado `openMS`.

## 🚀 Ejecución
Para correr el programa:

./openMS

Verás un menú como este:

1. Registrarse
2. Iniciar sesión
3. Ver canciones
0. Salir

## 📂 Archivos de datos

### data/users.txt
Simula los usuarios registrados, con el formato:

usuario,contraseña

### data/songs.txt
Simula el catálogo de canciones, con el formato:

Título,Artista

## 🔄 Limpieza
Para eliminar los archivos `.o` y el ejecutable:

make clean

## 📌 Estado actual
- [x] Registro de usuarios
- [x] Inicio de sesión
- [x] Visualización de canciones
- [ ] Reproducción de música (por simular)
- [ ] Playlists personalizadas
- [ ] Sistema de favoritos


## 🛠 Requisitos
- GCC (GNU Compiler Collection)
- make
- Linux/macOS (también puede compilarse en Windows con MinGW o WSL)
