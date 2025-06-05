# openMS

Reproductor de música por línea de comandos en C, sin sockets ni base de datos. Usa archivos `.txt` para simular almacenamiento.

## Compilación

```bash
make
```

## Ejecución

```bash
./build/server
./build/client
```

## Archivos de datos

- `data/users.txt`: `usuario:contraseña`
- `data/songs.txt`: `título:artista:archivo:duración`

## Limpieza

```bash
make clean
```

## Estado

- [x] Inicio de sesión (Autenticado)
- [x] Cerrar Sesión
- [x] Ver canciones
