import io;

typedef object {
    int num = 1;
    char* str = "default";
} NewType;

main()
{
    NewType* myObject = malloc(300);
    if (myObject.num == 0) {
        forever {
            print(myObject.str);
        }
    }
    else print("else");
}