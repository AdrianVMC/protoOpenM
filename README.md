# openMS

Reproductor de música por línea de comandos en C, sin sockets ni base de datos. Usa archivos `.txt` para simular almacenamiento.

## Compilación

```bash
make
```

## Ejecución

```bash
./openMS
```

## Archivos de datos

- `data/users.txt`: `usuario,contraseña`
- `data/songs.txt`: `título,artista`

## Limpieza

```bash
make clean
```

## Estado

- [x] Registro de usuarios
- [x] Inicio de sesión
- [x] Ver canciones
