// index_html.h
const char *index_html = R"EOF(
<!DOCTYPE html>
<html lang="en">
<html>
<head><style>body {
    font-family: Arial, sans-serif;
    background-color: #f2f2f2;
    margin: 0;
    padding: 0
}

h1 {
    text-align: center;
    color: #333
}

form {
    display: flex;
    justify-content: center;
    align-items: center;
    flex-direction: column;
    margin: 20px;
    background-color: #fff;
    padding: 20px;
    border-radius: 5px;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2)
}

input[type="text"] {
    max-width: 60%;
    padding: 10px;
    margin: 10px 0;
    border: 2px solid #ccc;
    border-radius: 5px
}

button[type="submit"] {
    background-color: #4287f5;
    color: #fff;
    border: none;
    padding: 10px 20px;
    border-radius: 5px;
    cursor: pointer
}

button[type="submit"]:hover {
    background-color: #285399
}

</style></head>
<body>
 <h1 style='text-align: center;'>Welcome Server</h1>
 <div style='display:flex; width:100%; justify-content: center;'>
 <form action='/submit' method='POST' style='max-width:50%; width:100%; font-weight: bold;'>Authenticate with your PIN: <input type='text' name='inputText'><br><button type='submit'>SUBMIT</button></form>
 </div>
</body>
</html>
)EOF";
