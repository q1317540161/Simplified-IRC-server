This server is implemented based on [RFC-1459](https://datatracker.ietf.org/doc/html/rfc1459). It provides clients to communicate using IRC standard client application (e.g. [WeeChat](https://weechat.org/)). 
Following shows how to use WeeChat as a client to interact with others. You can also use `nc` as a client to connect to the server to see the raw message from the server.

## Setup

1. Compile and run the server
    ```
    make
    ./server <port>
    ```
2. Install WeeChat client and run the command below, see the [user guide](https://weechat.org/files/doc/weechat/stable/weechat_user.en.html#install) for the installation 
    ```
    weechat --dir <path>/<to>/<desired>/<directory>
    /server <servername> <ip>/<port>
    /set irc.server.<servername>.username <username>
    /connect <servername>
    ```

## Supported Command
1. **Change your nickname** to <nickname>
    ```
    NICK <nickname>
    ```
2. **Register a user** in the network
    ```
    USER <username> <hostname> <servername> <realname>
    ```
3. **List all users' informations** in the network
    ```
    USERS
    ```
4. **List the channel's information**
    ```    
    LIST [<channelname>]
    ```
    If no <channelname> is given, list all the channels'     informations.
5. **Join the channel** named <channelname>
    ```
    JOIN #<channelname>
    ```
6. **Set the topic** of #<channelname> to <topic>
    ```
    TOPIC #<channelname> [<topic>]
    ```
    If no <topic> is given, show the current topic of #    <channelname>
7. **Leave the channel** named <channelname>
    ```
    PART #<channelname>
    ```
8. **List all users** in the channel named #<channelname>
    ```
    NAMES #<channelname>
    ```
9. **Send a message** to a channel named #<channelname >or user named <nickname>
    ```
    PRIVMSG <receiver> <text to send>
    ```
10. **Disconnect the session** to the server
    ```
    QUIT
    ```
11. **Check the session is still alive**
    ```
    PING
    ```
These are the standard IRC message as a client. 
    If you use WeeChat as a client, you have to check the [user guide](https://weechat.org/files/doc/weechat/stable/weechat_user.en.html) to see how to interact instead.
    
## Demonstration
As you connect to the server sucessfully, you will see the magic message. 
Below is the screenshot of weechat user interface.
Wish you having fun with our server!
    ![](https://hackmd.io/_uploads/HJXohTxhn.png)
