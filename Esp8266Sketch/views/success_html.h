// success_html.h
const char* success_html = R"EOF(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Success</title>
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

        .checkmark {
            width: 100px;
            height: 100px;
            border-radius: 50%;
            background-color: #4CAF50;
            color: #fff;
            display: flex;
            justify-content: center;
            align-items: center;
            margin: 0 auto 20px;
        }

        .checkmark i {
            font-size: 3em;
        }

        h1 {
            color: #333;
        }
    </style>
</head>

<body>
    <div class="container">
        <div class="checkmark"> <i class="fas fa-check"></i> </div>
        <h1>Success!</h1>
         <p id="dynamic-message"></p>
    </div>
</body>

<script>
  const urlParams = new URLSearchParams(window.location.search);  
  const dynamicMessage = urlParams.get('message') || '';
  document.getElementById('dynamic-message').textContent = dynamicMessage;
</script>

</html>
)EOF";