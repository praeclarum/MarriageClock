
// Raise the resolution when printing
resolution = 20;

function inch(x) { return x * 25.4; }

baseThickness = 4;
axleLength = 12;

printerDiameterError = 0.3;

// Uncomment different lines for prints
function main () {
    // return printTop();
    // return union(inners(), bottom());
    return complete();
    // return bottom();
    // return esp();
    // return box();
}

function complete() {
    let dist = 5;
    return union(
        top().translate([0,0,dist/2]),
        bottom().translate([0,0,-dist/2]));
}

function printTop() {
    return top().rotateX(0);
}

function top() {
    let c = cube({size:[1000,1000,1000]}).translate([-500, -500, -1000]);
    let t = difference(box(), c);
    
    let h = heart();
    let d = heart().scale([0.9, 0.9, 2.0]);
    let hh = difference(h, d);
    let lip = hh.scale([0.9, 0.9, 0.6]).translate([0,0,2]);
    
    let text1 = csgFromSegments(vectorText('H+J')).translate([-30,-16,8]);
    
    let screen = cube({size: [24,10,80],center:true}).translate([0,30,8]);
    let screenBorder = cube({size: [44,16,40]}).translate([-22,-8+30,8-35]);
    
    return difference(
        union(screenBorder, t, difference(union(lip), inners())),
        screen
        ,screenBorder
        );
}

function bottom() {
    let c = cube({size:[1000,1000,1000]}).translate([-500, -500, 0]);
    return difference(box(), c);
}

function box () {
  return difference(
    hollowHeart().translate([0,0,0]),
    inners()
  );
}

function inners () {
    return union(
        // heart().scale([0.95, 0.95, 0.95]),
        esp().translate([-5,0,0]).rotateZ(-45).translate([0,0,0]),
        battery().translate([0,26,-9])
        );
}

function hollowHeart() {
    let thick = 4;
    let h = heart();
    let d = heart().scale([0.9, 0.9, 0.5]);
    // return union(h, d);
    return difference(h, d);
}

function heart() {
    let thick = 50;
    let c = cylinder({r:30, h:thick, center:true, fn:resolution});
    let c1 = c.translate([20,30,0]);
    let c2 = c1.translate([-40,0,0]);
    let t =
        cube({size: [52,52,thick],center:true})
        .rotateZ(45)
        .scale([1,1.0,1])
        .translate([0,5,0])
        ;
    let round = sphere({r:1,center:true,fn:resolution})
        .scale([70,70,15])
        .translate([0,30,0]);
    return intersection(union(c1, c2, t), round);
}

function esp() {
    let c =
        cube({size: [52,28,12],center:true})
        ;
    let w =
        cube({size: [12,12,10],center:true})
        .translate([26+3,0,-1])
        ;
    let distX = 45.5;
    let distY = 17.5;
    let drillDepth = 1;
    let d = cylinder({r:(2 + printerDiameterError)/2, h:drillDepth, center:true})
        .translate([0,0,-6-drillDepth/2]);
    let d1 = d.translate([distX/2,distY/2,0]);
    let d2 = d.translate([distX/2,-distY/2,0]);
    let d3 = d.translate([-distX/2,distY/2,0]);
    let d4 = d.translate([-distX/2,-distY/2,0]);
    let r = union(c, w, d1, d2, d3, d4);
    return r;
}

function battery() {
    let c =
        cube({size: [52,35,8],center:true})
        ;
    let w =
        cube({size: [6,6,8],center:true})
        .translate([26+3,0,0])
        ;
    let r = union(c, w);
    return r;
}

function csgFromSegments (segments) {
  let output = [];
  segments.forEach(segment => output.push(
    rectangular_extrude(segment, { w:4, h:4 })
  ));
  return union(output);
}

