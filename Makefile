CXX = g++
CXXFLAGS = -std=c++11
LDFLAGS = -lboost_system -lboost_filesystem -lboost_thread -lssl -lcrypto -lpthread
TARGET = smart_mailer
SRC_DIR = src
SRC = $(SRC_DIR)/smart_mailer.cpp
PYTHON_DEPS = $(SRC_DIR)/pixel_server/requirements.txt

include .env

export SMTP_SERVER
export SMTP_DOMAIN
export EMAIL
export PASSWORD

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

install_python_deps: $(PYTHON_DEPS)
	pip install -r $(PYTHON_DEPS)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)
