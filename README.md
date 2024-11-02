## Build Dependencies/Installation

The smart_mailer reuiqres `gcc`, `boost`, and `openssl`

### Fedora

```bash
sudo dnf install gcc-c++ boost-devel openssl-devel
```

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install build-essential libboost-all-dev libssl-dev
```

### Environment Variables

For the smart_mailer to work, some environment variables need to be set. Follow `env.example` to set the required environment variables.

```bash
    cp env.example .env
```

### Python Dependencies

Python dependencies can be installed using pip. The Makefile will install the required dependencies.

```bash
  make install_python_deps
```

## Build/Run Instructions

To build the smart_mailer program, run

```bash
make
make run
```

To start the pixel tracking server, run:

```bash
python pixel_server.py
```

## Cleaning Up

To clean up the build files, run:

```bash
make clean
```
