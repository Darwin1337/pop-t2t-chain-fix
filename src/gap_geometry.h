/* Geometry policy: connect a unique, small, nearly collinear gap.
 * All bounds are relative to segment length; no world-height/FPS constant.
 */
typedef struct { float x,y,z; } Vec;
typedef union { float f; unsigned u; } FloatBits;
static int finite_scalar(float x) { FloatBits b; b.f=x; return (b.u&0x7f800000u)!=0x7f800000u; }
static int finite_vec(Vec a) { return finite_scalar(a.x)&&finite_scalar(a.y)&&finite_scalar(a.z); }
static Vec sub(Vec a,Vec b) { Vec r={a.x-b.x,a.y-b.y,a.z-b.z};return r; }
static float dot(Vec a,Vec b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static float distance2(Vec a,Vec b) { Vec d=sub(a,b);return dot(d,d); }
static Vec mix(Vec a,Vec b,float t) { Vec d=sub(b,a);Vec r={a.x+t*d.x,a.y+t*d.y,a.z+t*d.z};return r; }

/* Returns the selected endpoint only when projection clamped an outside point.
 * Reject lateral excursions and very short/large or non-finite geometry.
 */
static int outside_endpoint(Vec p,Vec projection,Vec a,Vec b,Vec *nearEnd,Vec *farEnd) {
    if (!finite_vec(p)||!finite_vec(projection)||!finite_vec(a)||!finite_vec(b))return 0;
    Vec axis=sub(b,a);float length2=dot(axis,axis);
    if (!finite_scalar(length2)||length2<0.0001f||length2>10000.0f)return 0;
    float t=dot(sub(p,a),axis)/length2;
    Vec n,f;
    if (t<0.0f){n=a;f=b;}else if(t>1.0f){n=b;f=a;}else return 0;
    if (distance2(projection,n)>length2*0.00000004f)return 0;
    *nearEnd=n;*farEnd=f;return 1;
}

/* Endpoint orientation may be reversed. Candidate continues away from selected.
 * Gap <=25% of shorter segment; axes/gap aligned within cosine~0.9995.
 * Contact must fall strictly inside connector and within a 0.5% length corridor.
 */
static int bridge_candidate(Vec p,Vec nearEnd,Vec farEnd,Vec a,Vec b,
                            Vec *candidate_near,Vec *result,float *gap2) {
    if (!finite_vec(p)||!finite_vec(nearEnd)||!finite_vec(farEnd)||!finite_vec(a)||!finite_vec(b))return 0;
    Vec n=a,f=b;
    if(distance2(nearEnd,b)<distance2(nearEnd,a)){n=b;f=a;}
    Vec selected=sub(farEnd,nearEnd),next=sub(f,n),gap=sub(n,nearEnd);
    float s2=dot(selected,selected),n2=dot(next,next),g2=dot(gap,gap);
    if(!finite_scalar(s2)||!finite_scalar(n2)||!finite_scalar(g2))return 0;
    float shorter=s2<n2?s2:n2;
    if(shorter<0.0001f||s2>10000.0f||n2>10000.0f||g2<shorter*0.00000004f||g2>shorter*0.0625f)return 0;
    float ds=dot(gap,selected),dn=dot(gap,next);
    if(ds>=0.0f||dn<=0.0f||ds*ds<g2*s2*0.999f||dn*dn<g2*n2*0.999f)return 0;
    float t=dot(sub(p,nearEnd),gap)/g2;
    if(!finite_scalar(t)||t<=0.0f||t>=1.0f)return 0;
    Vec q=mix(nearEnd,n,t);
    if(distance2(p,q)>shorter*0.000025f)return 0;
    *candidate_near=n;*result=q;*gap2=g2;return 1;
}
