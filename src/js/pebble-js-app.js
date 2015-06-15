var locationOptions = {
  enableHighAccuracy: true, 
  maximumAge: 10000, 
  timeout: 10000
};
currentPosition = null;

function locationSuccess(pos) {
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  if (currentPosition == null) {
      getCloseStops(pos.coords.latitude, pos.coords.longitude);
  }
  currentPosition = { 'x': pos.coords.latitude, 'y': pos.coords.longitude };
}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

Pebble.addEventListener('ready',
  function(e) {
    // Request current position
    console.log("Hello world! - Sent from your javascript application.");
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
    sendMsg({'0':7});
  }
);

Pebble.addEventListener('appmessage', function (e) {
    console.log(JSON.stringify(e.payload));
    if (e.payload[0] == "#GET STOPS") {
        if (currentPosition !== null) {
            getCloseStops(currentPosition.x,
                          currentPosition.y);    
        } else {
            /* tell pebble that we know shit right now */
        }
    } else {
        getLinesForStop(e.payload[0]);
    }
});

canSend = true;
msgQueue = [];
function sendMsg(data) {
    if (canSend) { 
        canSend = false;
        var forsure = setTimeout(function(){canSend=true;}, 5000);
        Pebble.sendAppMessage( data,
            function(e) {
                console.log('Successfully delivered message with transactionId=' + e.data.transactionId);
                canSend = true;
                clearTimeout(forsure);
                if (msgQueue.length > 0) {
                    sendMsg(msgQueue.shift());
                }
            },
            function(e) {
                console.log('Unable to deliver message with transactionId='
                  + e.data.transactionId
                  + ' Error is: ' + e.error.message);
                canSend = true;
                clearTimeout(forsure);
                if (msgQueue.length > 0) {
                    sendMsg(msgQueue.shift());
                }
            }
        );
        console.log(JSON.stringify(data));
    } else {
        msgQueue.push(data);
    }
}

function sqDist(pos) {
    return Math.pow(parseFloat(pos.x) - parseFloat(currentPosition.x), 2)
         + Math.pow(parseFloat(pos.y) - parseFloat(currentPosition.y), 2);
}

function getCloseStops(latitude, longitude) {
    var http = new XMLHttpRequest();
    http.open("GET",
            "http://busio.com.pl/Home/GetNearestStops?x=" + latitude + "&y=" + longitude + "&r=1000");
    http.onreadystatechange = function () {
        console.log(http.readyState + ' ' + http.status);
        if (http.readyState == 4 && http.status == 200) {
            console.log(http.responseText);
            var json = JSON.parse(http.responseText);
            json = json.sort(function(a, b) { return sqDist(a) - sqDist(b); });
            var result = {};
            result[0] = 1;
            var index = 1;
            var used = {};
            for (var i = 0; i < json.length; i++) {
                if (json[i].data.name !== undefined && !(json[i].data.name in used)) { 
                    result[index++] = json[i].data.name;
                    result[index++] = Math.sqrt(sqDist(json[i], currentPosition))*10000 + ' ';
                    used[json[i].data.name] = true;
                }
            }
            sendMsg(result);
        }
    } 
    http.send(null);
}

function getTimeStringFromJSon(json, maxLen, minHour) {
   var result = "";
   var current_minutes = new Date().getMinutes();
  
   for (var hour in json) {
      if ((minHour == -1 || hour >= minHour) && json[hour] !== '-') {
          var minutes = json[hour].split(' ');
          if(minHour > -1 && hour == minHour) {
              while(minutes.length > 0 && parseInt(minutes[0]) < current_minutes) {
                  minutes.shift();
              }
          }
          if (minutes.length > 0) {
              result += hour + ":" + minutes.join(' ') + ' ';  
          }
          
          if(result.length >= maxLen)
            return result;
      }
  }
  
  return result;
}

function getLinesForStop(stop) {
    var http = new XMLHttpRequest();
    http.open("GET",
            "http://busio.com.pl/Home/GetDeparturesForStop?stop=" + stop + "&delay=0");
    http.onreadystatechange = function () {
        console.log(http.readyState + ' ' + http.status);
        if (http.readyState == 4 && http.status == 200) {
            var json = JSON.parse(http.responseText);
            var index = 1;
            var current_hour = new Date().getHours();
            var current_minutes = new Date().getMinutes();
            var lines = [];
            for (var sname in json)
            {
                var time_str = "";
                
                time_str = getTimeStringFromJSon(json[sname], 25, current_hour);
                
                if (time_str.length > 0) { 
                  if(time_str.length < 22) {
                    time_str += getTimeStringFromJSon(json[sname], 25 - time_str.length, -1);
                  }
                  
                    lines.push([sname, time_str]);
                }
            }
            
            lines.sort(function(a, b) 
            { 
              var atime = a[1].split(' ')[0];
              var ahour = atime.split(':')[0];
              var aminute = atime.split(':')[1];
              
              var btime = b[1].split(' ')[0];
              var bhour = btime.split(':')[0];
              var bminute = btime.split(':')[1];
              
              if(ahour == bhour)
                return aminute - bminute;
                
              return ahour - bhour;
            });
            
            var result = {'0':2}
            lines.forEach(function(line) {
                result[index++] = line[0];
                result[index++] = line[1];
                // this means always
                if (Object.keys(result).length > 2) {
                    sendMsg(result);
                    result = {'0':2};
                }
            });
            sendMsg(result);
        }
    } 
    http.send(null);
}
