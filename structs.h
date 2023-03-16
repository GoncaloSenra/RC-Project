
struct admin {
    char user[100];
    char pass[100];
};

struct wallet {
	char name[100];
	int num;
};

struct stock {
    char name[100];
    double value;
};

struct market {
    char name[100];
    struct stock st[3];
};

struct user {
    char user[100];
    char pass[100];
    double saldo;
    char markets[2][100];
    char sub[2][100];
	struct wallet wal[6];
};


struct shared{
	struct admin adm;
	struct user usr[10];
	struct market mkt[2];
	int time;
};


