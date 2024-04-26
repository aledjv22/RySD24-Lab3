<h1 align="center" style="font-weight:bold;">
  Laboratorio 3: Transporte
</h1>
<p align="center" style="color:gray; font-size: 20px; font-weight:bold;">
  Cátedra de Redes y Sistemas Distribuidos
</p>

## Objetivos
- Leer, comprender y generar modelos de red en Omnet++.
- Analizar tráfico de red tasas de datos acotadas y tamaños de buffers limitados.
- Diseñar y proponer soluciones de control de congestión y flujo.

## Modelo de Colas
Se entrega como *kickstarter* un modelo de colas que consta de un generador (gen), una cola (queue) y un destino (sink) conectados en una red Network (definida en el archivo .ned).

<div align="center">
  <img src="https://i.ibb.co/ZgzKym6/Captura-desde-2024-04-26-00-52-13.png" alt="Imagen network">
</div>

En el archivo omnetpp.ini se configura una simulación de 200s donde gen crea y transmite paquetes con intervalos dados por una distribución exponencial de media configurable, y queue es capaz de atenderlos bajo una misma distribución.
```ini
sim-time-limit = 200s
Network.gen.generationInterval = exponential(1)
Network.queue.serviceTime = exponential(1)
```

El comportamiento de cada módulo se especifica en su respectiva clase en C++, declarada y definida en un único archivo de código ([Generator.cc](./Generator.cc), [Queue.cc](./Queue.cc) y [Sink.cc](./Sink.cc)). En particular, el módulo Sink toma métricas (.vec y .sca) de la demora de entrega de los paquetes.

## Tarea Análisis
El modelo de colas ignora nociones de capacidad (tasa de transferencia de datos y memoria de buffers). La primer tarea es modificar y extender el proyecto para hacer un **análisis** del impacto de estos parámetros en el tráfico de red.

Debido a que es la primera interacción con Omnet++, se provee una guía paso a paso, con snippets de código, que deberá ser complementada con el material de clases y consultas al manual de la herramienta¹.
> ¹Manual de Omnet++ disponible en https://omnetpp.org/doc/omnetpp/manual/

### Modificaciones en network.ned
El nodo Generator y Sink pasarán a ser parte de módulos compuestos denominados nodeTx y nodeRx. Deberán contener un buffer de transmisión y recepción que se podrá instanciar con el módulo de cola existente.

```c++
module NodeTx
{
  parameters:
    @display("i=block/routing");
  gates:
    output out;
  submodules:
    gen: Generator {
      parameters:
        @display("p=75,50");
    }
    queue: Queue {
      parameters:
        @display("p=75,190");
    }
  connections:
    gen.out --> queue.in;
    queue.out --> out;
}
```

```c++
module NodeRx
{
  parameters:
    @display("i=block/routing");
  gates:
    input in;
  submodules:
    sink: Sink {
      parameters:
        @display("p=75,50");
    }
    queue: Queue {
      parameters:
        @display("p=75,190");
    }
  connections:
    in --> queue.in;
    //case study 1
    //queue.out --> { datarate = 0.5Mbps; } --> sink.in;
    //case study 2
    queue.out --> { datarate = 1Mbps; } --> sink.in;
}
```

```c++
network Network
{
  @display("bgl=2");
  submodules:
    nodeTx: NodeTx {
      @display("p=30,30");
    }
    queue: Queue {
      @display("p=130,30");
    }
    nodeRx: NodeRx {
      @display("p=230,30");
    }
  connections:
    nodeTx.out --> { datarate = 1Mbps; delay = 100us; } --> queue.in;
    //case study 1
    //queue.out --> { datarate = 1Mbps; delay = 100us; } --> nodeRx.in;
    //case study 2
    queue.out --> { datarate = 0.5Mbps; delay = 100us; } --> nodeRx.in;
}
```

<div align="center">
  <img src="https://i.ibb.co/CBmzx3D/Captura-desde-2024-04-26-01-21-53.png" alt="network.ned">
</div>

Las conexiones deberán configurarse con tasas y demoras de transmisión para dos casos de
estudio específicos:
- Caso de estudio 1:
  - NodeTx a Queue: datarate = 1Mbps y delay = 100us
  - Queue a NodeRx: datarate = 1Mbps y delay = 100us
  - Queue a Sink: datarate = 0.5Mbps
- Caso de estudio 2:
  - NodeTx a Queue: datarate = 1Mbps y delay = 100us
  - Queue a NodeRx: datarate = 0.5Mbps y delay = 100us
  - Queue a Sink: datarate = 1Mbps

El módulo gen deberá tomar como parámetro el tamaño del paquete en Bytes (`packetByteSize`) que tendrá un valor de 12500 Bytes. Los mismos serán generados en intervalos de tiempos exponenciales con media de 100 ms.

Las queue serán limitados en tamaño de buffer (`bufferSize`), expresado en cantidad de paquetes. Se configurará con un valor máximo de 200, salvo la cola del nodo transmisor que se dejará arbitrariamente alta².
> ²El transmisor, en general, ya tiene en su memoria el contenido que desea transmitir, por lo que el tamaño de su buffer de salida no suele ser una limitante.

```ini
sim-time-limit = 200s
Network.nodeTx.gen.generationInterval = exponential(0.1)
Network.nodeTx.gen.packetByteSize = 12500
Network.queue.bufferSize = 200
Network.nodeRx.queue.bufferSize = 200
Network.nodeTx.queue.bufferSize = 2000000
```

### Modificaciones en clases de C++
Los objetos cMessage no tienen parámetros de tamaño en Bytes. Se deberá cambiar por objetos cPacket y configurar su tamaño en base al parámetro de configuración correspondiente.

```c++
cPacket *pkt;
pkt = new cPacket("packet");
pkt->setByteLength(par("packetByteSize"));
```

El tiempo de servicio de Queue deberá basarse en la duración de la transmisión del paquete una vez ya encolado (tiene en cuenta la tasa de datos configurada en la conexión).

```c++
// send
send(pkt, "out");
serviceTime = pkt->getDuration();
scheduleAt(simTime() + serviceTime, endServiceEvent);
```

Se deberá modificar el código de Queue para que controle que el tamaño del buffer no sobrepase el límite configurado. De estar lleno, el nuevo paquete se ignorará y se borrará.

```c++
// check buffer limit
if (buffer.getLength() >= par("bufferSize").longValue()) {
  // drop the packet
  delete msg;
  this->bubble("packet dropped");
  packetDropVector.record(1);
} else {
  // enqueue the packet
  buffer.insert(msg);
  bufferSizeVector.record(buffer.getLength());
  // if the server is idle
  if (!endServiceEvent->isScheduled()) {
    // start the service now
    scheduleAt(simTime() + 0, endServiceEvent);
  }
}
```

Se deberán agregar nuevas métricas para el análisis en Queue, una que mida la cantidad de
paquetes en el buffer, y otra que mida la cantidad de paquetes descartados por buffer saturado.
  
```c++
cOutVector bufferSizeVector;
cOutVector packetDropVector;
```

### Experimentos y Preguntas
Se deberá correr simulaciones paramétricas para cada caso de estudio, variando el intervalo de generación de paquetes (`generationInterval`) entre 0.1 y 1 en los pasos que el grupo crea adecuado para responder las preguntas planteadas.
Se **deberá** generar algunas gráficas representativas de la utilización de cada una de las 3 queue para los caso de estudios planteados.
Se **sugiere** crear una gráfica de carga transmitida (eje x) vs. carga recibida (eje y), ambas
expresadas en paquetes por segundo (**ver Figura 6-19 del libro Tanenbaum**). En caso de que
haya pérdidas de paquetes también se sugiere medir y comparar estadísticas de ese
fenómeno.

Responda y justifique
  1. ¿Qué diferencia observa entre el caso de estudio 1 y 2? ¿Cuál es la fuente limitante en cada uno? Investigue sobre la diferencia entre control de flujo y control de congestión (**ver Figura 6-22 del libro Tanenbaum**).

