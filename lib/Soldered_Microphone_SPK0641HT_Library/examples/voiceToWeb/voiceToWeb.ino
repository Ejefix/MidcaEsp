#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "Soldered-Microphone-SPK0641HT.h"

const char* ssid = "RP2350_Audio_Stream";
const char* password = "123456789";

WebServer server(80);
WebSocketsServer webSocket(81);

// Audio streaming parameters - optimized for real-time
const int SAMPLE_RATE = 16000;
const int CHANNELS = 1;
const int BIT_DEPTH = 16;
const int SAMPLES_PER_BUFFER = 160; // 10ms of audio (16000/1000*10)
const int BYTES_PER_BUFFER = SAMPLES_PER_BUFFER * 2;

// Double buffering for smooth streaming
uint8_t audioBuffer1[BYTES_PER_BUFFER];
uint8_t audioBuffer2[BYTES_PER_BUFFER];
uint8_t* currentBuffer = audioBuffer1;
uint8_t* sendBuffer = audioBuffer2;
int bufferIndex = 0;
bool bufferReady = false;

unsigned long lastAudioSend = 0;
const int AUDIO_SEND_INTERVAL = 10; // Send every 10ms

Microphone mic;
unsigned long lastDebugTime = 0;
int clientCount = 0;
int totalPacketsSent = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // HTTP server
  server.on("/", handleRoot);
  server.begin();

  // WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Initialize microphone
  constexpr int PDM_CLK = 4;
  constexpr int PDM_DIN = 7;

  Serial.println("Initializing microphone...");
  mic.begin(PDM_DIN, PDM_CLK, SAMPLE_RATE, BIT_DEPTH, SAMPLES_PER_BUFFER);
  Serial.println("Microphone initialized successfully");
  mic.setHPF(true);
  mic.setGainDb(10.0f);
  
  Serial.printf("Buffer size: %d samples (%d bytes), Interval: %dms\n", 
                SAMPLES_PER_BUFFER, BYTES_PER_BUFFER, AUDIO_SEND_INTERVAL);
}

void loop() {
  server.handleClient();
  webSocket.loop();
  readMicData();
  handleAudioStreaming();
  
  // Debug output
  if (millis() - lastDebugTime > 1000) {
    Serial.printf("Clients: %d, Packets sent: %d, Buffer ready: %d\n", 
                  clientCount, totalPacketsSent, bufferReady);
    lastDebugTime = millis();
  }
}

// Serve HTML page
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>Audio Stream</title>
    <style>
      body { font-family: Arial, sans-serif; margin: 20px; }
      button { padding: 10px 20px; font-size: 16px; }
      #status { margin: 10px 0; font-weight: bold; }
      #debug { color: #666; }
    </style>
  </head>
  <body>
    <h1>RP2350 Live Microphone Audio</h1>
    <button onclick="startAudio()">Start Audio</button>
    <div id="status">Status: Disconnected</div>
    <div id="debug">Debug: Ready to connect</div>
    <div id="stats">Packets received: 0</div>
    
    <script>
      let audioContext;
      let websocket;
      let audioQueue = [];
      let isPlaying = false;
      let receivedBuffers = 0;
      let lastPacketTime = 0;
      let jitterBuffer = [];
      const TARGET_BUFFER_TIME = 100; // ms of buffering for smooth playback
      
      function updateDebug(message) {
        document.getElementById('debug').textContent = 'Debug: ' + message;
      }
      
      function updateStats() {
        document.getElementById('stats').textContent = 'Packets received: ' + receivedBuffers;
      }
      
      function startAudio() {
        updateDebug('Starting audio...');
        
        // Create audio context
        audioContext = new (window.AudioContext || window.webkitAudioContext)({
          sampleRate: 16000,
          latencyHint: 'playback'
        });
        
        // Create WebSocket connection
        const ip = ')rawliteral";
  html += WiFi.softAPIP().toString();
  html += R"rawliteral(';
        websocket = new WebSocket('ws://' + ip + ':81');
        
        websocket.binaryType = 'arraybuffer';
        document.getElementById('status').textContent = 'Status: Connecting...';
        
        websocket.onopen = function() {
          document.getElementById('status').textContent = 'Status: Connected';
          updateDebug('WebSocket connected, starting playback...');
          startPlayback();
        };
        
        websocket.onclose = function() {
          document.getElementById('status').textContent = 'Status: Disconnected';
          updateDebug('WebSocket disconnected');
        };
        
        websocket.onerror = function(error) {
          updateDebug('WebSocket error');
        };
        
        websocket.onmessage = function(event) {
          receivedBuffers++;
          updateStats();
          
          if (event.data instanceof ArrayBuffer) {
            const int16Data = new Int16Array(event.data);
            const now = Date.now();
            
            // Calculate network jitter
            const packetTime = now - lastPacketTime;
            lastPacketTime = now;
            
            // Add to jitter buffer
            jitterBuffer.push({
              data: int16Data,
              timestamp: now
            });
            
            // If we have enough buffered audio, start playback
            if (jitterBuffer.length > 5 && !isPlaying) {
              processJitterBuffer();
            }
          }
        };
      }
      
      function startPlayback() {
        // Create script processor for continuous playback
        const bufferSize = 256;
        const processor = audioContext.createScriptProcessor(bufferSize, 1, 1);
        
        let playbackPosition = 0;
        let currentBuffer = null;
        let bufferPosition = 0;
        
        processor.onaudioprocess = function(event) {
          const output = event.outputBuffer.getChannelData(0);
          
          // Fill output buffer with audio data
          for (let i = 0; i < bufferSize; i++) {
            if (currentBuffer && bufferPosition < currentBuffer.length) {
              output[i] = currentBuffer[bufferPosition++] / 32768.0;
            } else {
              // Get next buffer from jitter buffer
              if (jitterBuffer.length > 0) {
                const next = jitterBuffer.shift();
                currentBuffer = next.data;
                bufferPosition = 0;
                if (currentBuffer && bufferPosition < currentBuffer.length) {
                  output[i] = currentBuffer[bufferPosition++] / 32768.0;
                } else {
                  output[i] = 0;
                }
              } else {
                output[i] = 0;
              }
            }
          }
        };
        
        processor.connect(audioContext.destination);
        isPlaying = true;
        updateDebug('Playback started');
      }
      
      function processJitterBuffer() {
        // Sort buffer by timestamp to handle out-of-order packets
        jitterBuffer.sort((a, b) => a.timestamp - b.timestamp);
      }
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// WebSocket events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.printf("[WS] Client %u connected\n", num);
      clientCount++;
      break;
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %u disconnected\n", num);
      clientCount--;
      break;
  }
}

// Read microphone samples
void readMicData() {
  if (bufferReady) return; // Don't read if buffer is ready to send
  
  int16_t samples[SAMPLES_PER_BUFFER];
  int samplesRead = mic.read(samples, SAMPLES_PER_BUFFER);
  
  if (samplesRead > 0) {
    // Convert samples to bytes
    for (int i = 0; i < samplesRead; i++) {
      if (bufferIndex * 2 + 1 < BYTES_PER_BUFFER) {
        currentBuffer[bufferIndex * 2] = samples[i] & 0xFF;
        currentBuffer[bufferIndex * 2 + 1] = (samples[i] >> 8) & 0xFF;
        bufferIndex++;
      }
    }
    
    if (bufferIndex >= SAMPLES_PER_BUFFER) {
      bufferReady = true;
      bufferIndex = 0;
    }
  }
}

// Send audio to connected clients
void handleAudioStreaming() {
  unsigned long now = millis();
  
  if (clientCount > 0 && bufferReady && (now - lastAudioSend >= AUDIO_SEND_INTERVAL)) {
    // Swap buffers
    uint8_t* temp = sendBuffer;
    sendBuffer = currentBuffer;
    currentBuffer = temp;
    
    // Send the audio data
    webSocket.broadcastBIN(sendBuffer, BYTES_PER_BUFFER);
    
    bufferReady = false;
    lastAudioSend = now;
    totalPacketsSent++;
    
    if (totalPacketsSent % 100 == 0) {
      Serial.printf("[AUDIO] Sent packet %d\n", totalPacketsSent);
    }
  }
}