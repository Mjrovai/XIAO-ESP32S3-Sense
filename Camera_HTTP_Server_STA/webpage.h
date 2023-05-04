


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
        body {
          background-color: #0072b5; 
          text-align:center; 
          font-family: Arial, Helvetica, Sans-Serif; 
          Color: white; 
        }
  </style>
</head>
<body>
  <div id="container">
    <h2>XIAO ESP32S3-Sense Image Capture</h2>

    <p>
      <button onclick="rotatePhoto();">ROTATE</button>
      <button onclick="capturePhoto()">CAPTURE</button>
      <button onclick="savePhoto()">SAVE</button>
    </p>
    <p>[New Captured Image takes 5 seconds to appear]</p>
    
  </div>
  <div><img src="/saved_photo" id="photo" width="70%"></div>
        <hr>
      <h5> @2023 Marcelo Rovai - MJRoBot.org </h5>
</body>
<script>
  var deg = 0;
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
    wait(5000);
    location.reload();
  }

  function savePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/save", true);
    xhr.send();
    wait(5000);
    location.reload();
  }
  
  function rotatePhoto() {
    var img = document.getElementById("photo");
    deg += 180;
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; }
    else{ document.getElementById("container").className = "hori"; }
    img.style.transform = "rotate(" + deg + "deg)";
  }
  function isOdd(n) { return Math.abs(n % 2) == 1; }

  function wait(ms){
   var start = new Date().getTime();
   var end = start;
   while(end < start + ms) {
     end = new Date().getTime();
  }
}
</script>
</html>)rawliteral";
