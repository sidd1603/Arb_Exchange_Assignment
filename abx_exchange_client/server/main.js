const net = require('net');

const packetData = {
    packetStream: [
        { symbol: "AAPL", buysellindicator: "B", quantity: 50, price: 100, packetSequence: 1 },
        { symbol: "AAPL", buysellindicator: "B", quantity: 30, price: 98, packetSequence: 2 },
        { symbol: "AAPL", buysellindicator: "S", quantity: 20, price: 101, packetSequence: 3 },
        { symbol: "AAPL", buysellindicator: "S", quantity: 10, price: 102, packetSequence: 4 },
        { symbol: "META", buysellindicator: "B", quantity: 40, price: 50, packetSequence: 5 },
        { symbol: "META", buysellindicator: "S", quantity: 30, price: 55, packetSequence: 6 },
        { symbol: "META", buysellindicator: "S", quantity: 20, price: 57, packetSequence: 7 },
        { symbol: "MSFT", buysellindicator: "B", quantity: 25, price: 150, packetSequence: 8 },
        { symbol: "MSFT", buysellindicator: "S", quantity: 15, price: 155, packetSequence: 9 },
        { symbol: "MSFT", buysellindicator: "B", quantity: 20, price: 148, packetSequence: 10 },
        { symbol: "AMZN", buysellindicator: "B", quantity: 10, price: 3000, packetSequence: 11 },
        { symbol: "AMZN", buysellindicator: "B", quantity: 5, price: 2999, packetSequence: 12 },
        { symbol: "AMZN", buysellindicator: "S", quantity: 15, price: 3020, packetSequence: 13 },
        { symbol: "AMZN", buysellindicator: "S", quantity: 10, price: 3015, packetSequence: 14 }
    ]
};

const PACKET_CONTENTS = [
    { name: "symbol", type: "ascii", size: 4 },
    { name: "buysellindicator", type: "ascii", size: 1 },
    { name: "quantity", type: "int32", size: 4 },
    { name: "price", type: "int32", size: 4 },
    { name: "packetSequence", type: "int32", size: 4 }
];

const createPayloadToSend = (packet) => {
    let offset = 0;
    const buffer = Buffer.alloc(17);  // Total size: 4 + 1 + 4 + 4 + 4 = 17 bytes

    PACKET_CONTENTS.forEach(({ name, type }) => {
        if (type === "int32") {
            offset = buffer.writeInt32BE(packet[name], offset);
        } else {
            offset += buffer.write(packet[name], offset, 'ascii');
        }
    });

    return buffer;
};

const server = net.createServer((socket) => {
    console.log('Client connected.');

    socket.on('data', (data) => {
        const callType = data.readInt8(0);
        const resendSeq = data.readInt8(1);

        if (callType === 1) {
            packetData.packetStream.forEach((packet) => {
                if (Math.random() > 0.75) return;  // Randomly skip some packets
                const payload = createPayloadToSend(packet);
                socket.write(payload);
            });
            socket.end();
            console.log('Packets sent. Client disconnected.');
        } else if (callType === 2) {
            const packet = packetData.packetStream.find((p) => p.packetSequence === resendSeq);
            const payload = createPayloadToSend(packet);
            socket.write(payload);
            console.log('Packet resent.');
        }
    });

    socket.on('end', () => {
        console.log('Client disconnected.');
    });
});

server.listen(3000, () => {
    console.log('TCP server started on port 3000.');
}); 