function* upToN(n) {
    var index = 0;
    while (index < n)
          yield index++;
}


var svgContainer = d3.select("svg")
    .attr("width", 800)
    .attr("height", 200);

var ledStrip = svgContainer.append("g");

ledStrip.selectAll("rect")
    .data(Array.from(upToN(31)))
    .enter()
    .append("rect")
    .attr("x", function (d, i) { return i * 22; })
    .attr("y", 10)
    // .attr("rx", function(d, i) {return i*10;})
    // .attr("ry", 10)
    .attr("width", 20)
    .attr("height", 20)
    .style("fill", "#000000");

var ledSquares = ledStrip.selectAll("rect");

function updateColors(data) {
    console.log(data)
    ledSquares.data(data)
          .style("fill", function (d) { return "#" + d })
}

function arraysEqual(a, b) {
    if (a === b) return true;
    if (a == null || b == null) return false;
    if (a.length != b.length) return false;

    // If you don't care about the order of the elements inside
    // the array, you should sort both arrays here.
    // Please note that calling sort on an array will modify that array.
    // you might want to clone your array first.

    for (var i = 0; i < a.length; ++i) {
          if (a[i] !== b[i]) return false;
    }
    return true;
}

// the data produced by the simulator produces a lot of duplicate data
// and there's no point in updating the virtual strip if there's no
// new data coming in
function cull_duplicates(led_data) {
    var cur_leds = [];
    var index = 0;
    while (index < led_data.length) {
          if (!arraysEqual(cur_leds, led_data[index].leds)) {
                cur_leds = led_data[index].leds;
                index = index + 1;
          } else {
                led_data.splice(index, 1);
          }
    }

};

var led_data = null;
var start = null;
var led_index = 0;
function animate(timestamp) {
    if(!start){ 
          start = timestamp;
          console.log("start at " + start);
          };
    if(led_index < led_data.length){
          var update_time = led_data[led_index].time + start;
          if(timestamp > update_time){
                updateColors(led_data[led_index].leds);
                led_index = find_next_index(led_index);
          }
          window.requestAnimationFrame(animate);
    } else{
          //console.log("done at " + timestamp);
          led_index = 0;
          start = timestamp;
          window.requestAnimationFrame(animate);
    }
};

function find_next_index(cur_index){
    now = performance.now();
    while(cur_index < led_data.length){
          var next_update_time = led_data[cur_index].time + start;
          if(next_update_time > now){
                return cur_index;
          }
          //console.log("skipped an update");
          cur_index = cur_index + 1;
    }
}

// this function takes csv headers and builds the javascript object
// for a single row
function row(d) {
    response = { time: d.t/1000000, length: d.l, leds: [] }
    for (i = 0; i < response.length; i++) {
          response.leds.push(d[i]);
    }
    return response;
}

// loads csv data into javascript object for easier handling
d3.csv("static/rainbow.csv", row).then(function (data) {
    cull_duplicates(data);
    led_data = data;
    window.requestAnimationFrame(animate);
});