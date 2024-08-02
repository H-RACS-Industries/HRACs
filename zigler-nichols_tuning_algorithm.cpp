long currentMillis, previousMillis;
double d2ValueC, lastDValueC, errC, lastErrC, dValueC;
float interval;
float l, t;
//FIX
void subCalibrationFindInflectionPoint (double target) { //the target is just a reasonable placeholder because the tuning depends on the change, not the value itself.
    moveDeg(255);
    interval = 1000; //set interval (in miliseconds)
    while (d2Value != 0){
        //add stuff to change currentmillis and previous millis

        if (currentMillis - previousMillis >= interval) {//loop every interval
            //calculate value of current error, derivative value, and 2nd derivative value
            current = readTemp();
            errC = target - current;
            dValueC = (errC - lastErrC) / interval;
            d2ValueC = (dValue - lastDValueC) / interval; 
            //update the last value of both the error and derivative value
            lastDValueC = dValueC; 
            lastErrC = errC;
        }
    }
}
void subCalibrationCalculateLandT() {
    if (dValue == 0) {
        //find the x-intercept of the tangent line of the temperature graph
        // y - y1 = m(x-x1) sketchy math @@
        // -current = dValue(x-currentMillis);
        l = dValue * currentMillis - current;

        //find the difference between the x-value where the tangent line of the temperature intersects the target temperature and the x-intercept of the tangent line
        //target = dValue * (x-currentMillis) + current;
        t = (target + dValue * currentMillis - current) - l;
        kp = 1.2 * t / l;
        ki = 2 * l;
        kd = 0.5 * l;
    }   
}
void mainCalibration() {
    subCalibrationFindInflectionPoint ();
    subCalibrationCalculateLandT();
}
