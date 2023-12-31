// copyIp_html.h

const char copyIp_html[] PROGMEM = R"EOF(
    <!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Public IP</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.3/css/all.min.css">

    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #282c34;
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }

        .container {
            background: #fff;
            padding: 20px;
            border-radius: 5px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            width: 300px;
            text-align: center;
            position: relative;
        }

        .container h1 {
            color: #1A73E8;
            margin: 0;
            padding: 0;
        }

        .input-group {
            margin: 30px 0 0 0;
        }

        .input-group label {
            display: block;
            text-align: left;
            margin: 5px;
        }

        .input {
            padding: 10px;
            margin: 5px 0;
            border: 1px solid #ccc;
            border-radius: 3px;
            text-align: center;
        }

        .input-group button[type="submit"] {
            background: #333;
            color: #fff;
            cursor: pointer;
            width: 100%;
            font-size: medium;
            font-weight: bold;
        }

        .input-group button[type="submit"]:hover {
            background: #555;
        }

        .copy-button {
            position: absolute;
            top: 10px;
            right: 10px;
            background: transparent;
            color: #383838;
            border: none;
            padding: 5px;
            border-radius: 3px;
            cursor: pointer;
        }

        .copy-button i {
            font-size: x-large;
        }

        /* Add tooltip style */
        .tooltip {
            position: absolute;
            background: #1A73E8;
            color: #fff;
            margin: 0px 10px;
            padding: 5px 10px;
            border-radius: 5px;
            opacity: 0;
            transition: opacity 0.3s;
        }

        .tooltip::after {
            content: " ";
            position: absolute;
            top: 50%;
            right: 100%;
            /* To the left of the tooltip */
            margin-top: -5px;
            border-width: 5px;
            border-style: solid;
            border-color: transparent #1A73E8 transparent transparent;
        }

        .tooltip.show {
            opacity: 1;
        }
    </style>
</head>

<body>
    <div class="container">
        <h1 id="ipAddress"></h1>
        <button class="copy-button" onclick="copyToClipboard()">
            <i class="far fa-clone"></i>
            <span class="tooltip" id="copyTooltip">Copied!</span>
        </button>
        <form action="myAction" method="POST">
            <div class="input-group">
                <button class="input" type="submit">SUBMIT</button>
            </div>
        </form>
    </div>

    <script>
        // Get the URL parameters
        const urlParams = new URLSearchParams(window.location.search);

        // Extract the value of the 'ip' parameter from the URL
        const ipAddress = urlParams.get('ip');

        // Get the paragraph element by ID
        const paragraph = document.getElementById('ipAddress');

        // Assign the extracted IP address to the paragraph content
        if (ipAddress) {
            paragraph.textContent += ipAddress;
        } else {
            paragraph.textContent += 'IP address not found in URL.';
        }

        function copyToClipboard() {
            const ipAddress = document.getElementById("ipAddress");
            const textArea = document.createElement("textarea");
            textArea.value = ipAddress.innerText;
            document.body.appendChild(textArea);
            textArea.select();
            document.execCommand('copy');
            document.body.removeChild(textArea);

            // Show the tooltip
            const tooltip = document.getElementById("copyTooltip");
            tooltip.classList.add("show");
            setTimeout(() => {
                tooltip.classList.remove("show");
            }, 1200); // Hide the tooltip after 1.2 seconds
        }
    </script>
</body>

</html>
)EOF";