// MiniC program to test addition
extern int print_int(int X);

int addition(int n, int m){
// void addition(){
	int result;
	// result = 10 + 32;
  result = n+m;
  

  if(n == 4) {
    print_int(n+m);
  }
  else {
    print_int(n*m);
  }

  return result;
}

void check(){
  int a;
  int b;
  int c;
  a = 10;
  b = 10;
  c = addition(b,a);
  return ;
}