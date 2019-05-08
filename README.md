# Messenger
Simple messenger like app written in C designed to work on Windows.

This app consists of 3 projects. One for the Server, which manages incoming connections and processes the requests, one for the Client, which works like a normal console application, sending requests to the server and listening for response, and CommunicationLibraty, which facilitates the communication between the Server and multiple Clients.

The main goals for this app are:

1. Send data from a Client to the Server and back.
2. Client register and login.
3. Sending a message from a Client to another registered Client.
4. Sending a message from a Client to all other registered Clients.
5. Client logout.
6. Ability to see all logged in Clients.
7. See the message history.

For now, all persistance files are saved on the D:\ drive because administrator privileges are not required to read/write them. This will break the application if you don't have a D:\ drive. I will modify this once everything else it's working normally.

Everything should be functional, as well as making sure that the app doesn't crash and all errors are handled accordingly by implementing a fail safe system.
