#include <thread>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

const int TAM = 21;
const int MAX_RECORDES = 200;
const int VAZIO = 0, PAREDE = 1, JOGADOR = 2, BLOCO = 3;
const int ALAVANCA = 4, SAIDA = 5, PORTA_A = 6, PORTA_B = 7;

struct Posicao { int lin = -1, col = -1; };
struct Jogo {
    int cenario[TAM][TAM]{};
    int n = 0, orientacao = 0, movimentos = 0, rotacoes = 0, numeroMapa = 0;
    Posicao jogador;
    int sobJogador = VAZIO;
    string titulo;
};
struct Recorde { string nome; int mapa = 0, movimentos = 0, rotacoes = 0; };

template <typename T> void trocar(T &a, T &b) { T t = a; a = b; b = t; }

#ifdef _WIN32
char lerTecla() { return static_cast<char>(_getch()); }
void limparTela() { system("cls"); }
#else
char lerTecla() {
    termios oldt{}, newt{}; tcgetattr(STDIN_FILENO, &oldt); newt = oldt;
    newt.c_lflag &= static_cast<unsigned>(~(ICANON | ECHO)); tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    char c = 0; ssize_t lidos = read(STDIN_FILENO, &c, 1); (void)lidos;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); return c;
}
void limparTela() { cout << "\033[2J\033[H"; }
#endif

bool portaEstaFechada(int celula, int orientacao) {
    if (celula == PORTA_A) return orientacao == 0 || orientacao == 2;
    if (celula == PORTA_B) return orientacao == 1 || orientacao == 3;
    return false;
}
bool celulaLivre(int c, int o) {
    return c == VAZIO || c == ALAVANCA || c == SAIDA ||
           ((c == PORTA_A || c == PORTA_B) && !portaEstaFechada(c, o));
}
bool sustenta(int c, int o) {
    return c == PAREDE || c == BLOCO ||
           ((c == PORTA_A || c == PORTA_B) && portaEstaFechada(c, o));
}
void limpaCenario(Jogo &j) {
    for (int i=0;i<TAM;i++) for (int k=0;k<TAM;k++) j.cenario[i][k]=VAZIO;
}
void copiaJogo(const Jogo &a, Jogo &b) { b = a; }

void aplicaGravidade(Jogo &j, bool &esmagado) {
    esmagado = false;
    bool mudou;
    do {
        mudou = false;
        for (int i=j.n-2;i>=0;i--) for (int c=0;c<j.n;c++) {
            if (j.cenario[i][c] != BLOCO) continue;
            int fim=i;
            for (int k=i+1;k<j.n;k++) {
                int cel=j.cenario[k][c];
                if (cel == JOGADOR) { esmagado=true; break; }
                if (cel == VAZIO || ((cel==PORTA_A||cel==PORTA_B)&&!portaEstaFechada(cel,j.orientacao))) { fim=k; continue; }
                break;
            }
            if (fim!=i) { j.cenario[fim][c]=BLOCO; j.cenario[i][c]=VAZIO; mudou=true; }
        }
    } while (mudou && !esmagado);
}
void giraMatriz(Jogo &j, bool direita) {
    int aux[TAM][TAM]{};
    for (int i=0;i<j.n;i++) for (int c=0;c<j.n;c++)
        if (direita) aux[c][j.n-1-i]=j.cenario[i][c];
        else aux[j.n-1-c][i]=j.cenario[i][c];
    for (int i=0;i<j.n;i++) for (int c=0;c<j.n;c++) j.cenario[i][c]=aux[i][c];
    j.orientacao = direita ? (j.orientacao+1)%4 : (j.orientacao+3)%4;
    j.jogador = {-1,-1};
    for (int i=0;i<j.n;i++) for (int c=0;c<j.n;c++) if (j.cenario[i][c]==JOGADOR) j.jogador={i,c};
}
bool estaSobreAlavanca(const Jogo &j) { return j.sobJogador == ALAVANCA; }
bool venceu(const Jogo &j) { return j.sobJogador == SAIDA; }

bool processaTecla(Jogo &j, char tecla, string &mensagem) {
    tecla = static_cast<char>(toupper(static_cast<unsigned char>(tecla)));
    mensagem.clear();
    if (tecla == 'Q' || tecla == 'E') {
        if (!estaSobreAlavanca(j)) { mensagem="Voce precisa estar sobre uma alavanca."; return true; }
        giraMatriz(j, tecla=='E'); j.rotacoes++;
        bool esmagado=false; aplicaGravidade(j, esmagado);
        if (esmagado) { mensagem="Derrota: jogador esmagado."; return false; }
        return true;
    }
    if (tecla!='W' && tecla!='A' && tecla!='S' && tecla!='D') { mensagem="Tecla invalida."; return true; }
    int dl=0, dc=0; if(tecla=='W')dl=-1; if(tecla=='S')dl=1; if(tecla=='A')dc=-1; if(tecla=='D')dc=1;
    int nl=j.jogador.lin+dl, nc=j.jogador.col+dc;
    if(nl<0||nl>=j.n||nc<0||nc>=j.n) return true;
    int destino=j.cenario[nl][nc];
    if(!celulaLivre(destino,j.orientacao)) return true;
    j.cenario[j.jogador.lin][j.jogador.col]=j.sobJogador;
    j.sobJogador=destino; j.jogador={nl,nc}; j.cenario[nl][nc]=JOGADOR; j.movimentos++;
    bool esmagado=false; aplicaGravidade(j,esmagado);
    if(esmagado) { mensagem="Derrota: jogador esmagado."; return false; }
    return true;
}

bool leLinhaCSV(const string &linha, int n, int saida[]) {
    stringstream ss(linha), campo; string item; int qtd=0;
    while(getline(ss,item,',')) {
        if(item.empty() || qtd>=n) return false;
        try { size_t p=0; int v=stoi(item,&p); if(p!=item.size()||v<0||v>7)return false; saida[qtd++]=v; }
        catch(...) { return false; }
    }
    return qtd==n;
}
bool validaEstabilidade(const Jogo &inicial) {
    Jogo copia; copiaJogo(inicial,copia); bool esmagado=false; aplicaGravidade(copia,esmagado);
    if(esmagado) return false;
    for(int i=0;i<inicial.n;i++)for(int c=0;c<inicial.n;c++) if(inicial.cenario[i][c]!=copia.cenario[i][c]) return false;
    return true;
}
bool carregaMapa(int numero, Jogo &j, string &erro) {
    string nome="mapa"+to_string(numero)+".csv"; ifstream arq(nome); if(!arq){erro="arquivo inexistente";return false;}
    string linha; int n;
    if(!getline(arq,linha)){erro="dimensao ausente";return false;}
    try { size_t p=0; n=stoi(linha,&p); if(p!=linha.size())throw 1; } catch(...) {erro="dimensao nao numerica";return false;}
    if(n<11||n>TAM){erro="dimensao fora do intervalo 11..21";return false;}
    if(!getline(arq,j.titulo)||j.titulo.empty()){erro="titulo ausente";return false;}
    Jogo novo; novo.n=n; novo.numeroMapa=numero; novo.titulo=j.titulo; limpaCenario(novo);
    int jogadores=0, saidas=0;
    for(int i=0;i<n;i++) { if(!getline(arq,linha)){erro="linhas insuficientes";return false;} int vals[TAM]{}; if(!leLinhaCSV(linha,n,vals)){erro="linha com valores invalidos ou colunas incorretas";return false;} for(int c=0;c<n;c++){novo.cenario[i][c]=vals[c]; if(vals[c]==JOGADOR){jogadores++;novo.jogador={i,c};}if(vals[c]==SAIDA)saidas++;} }
    if(jogadores!=1){erro="quantidade de jogadores diferente de 1";return false;} if(saidas!=1){erro="quantidade de saidas diferente de 1";return false;}
    if (getline(arq,linha)) { erro="linhas extras apos a matriz"; return false; }
    novo.orientacao=0; novo.sobJogador=VAZIO; if(!validaEstabilidade(novo)){erro="mapa instavel";return false;}
    j=novo; return true;
}
int quantidadeMapas() { int n=0; while(true){ifstream a("mapa"+to_string(n+1)+".csv");if(!a)break;n++;}return n; }

void desenha(const Jogo &j) {
    ostringstream tela;
    tela << "\033[H\033[2KLABIRINTO GIRATORIO | Mapa " << j.numeroMapa << " - " << j.titulo << "\n";
    tela << "\033[2KOrientacao: " << j.orientacao*90 << " graus | Movimentos: " << j.movimentos
         << " | Rotacoes: " << j.rotacoes << "\n\n";
    for(int i=0;i<j.n;i++) {
        tela << "\033[2K";
        for(int c=0;c<j.n;c++) {
            char x=' ';
            switch(j.cenario[i][c]) {
                case PAREDE:x='#';break; case JOGADOR:x='@';break; case BLOCO:x='O';break;
                case ALAVANCA:x='A';break; case SAIDA:x='S';break;
                case PORTA_A:x=portaEstaFechada(PORTA_A,j.orientacao)?'=':':';break;
                case PORTA_B:x=portaEstaFechada(PORTA_B,j.orientacao)?'|':';';break;
            }
            tela << x << x;
        }
        tela << '\n';
    }
    tela << "\033[2K\nWASD mover | Q/E girar na alavanca | R reiniciar | ESC voltar\n\033[J";
    cout << tela.str() << flush;
}

bool salvaJogo(const Jogo &j) {
    ofstream a("jogo_salvo.csv"); if (!a) return false;
    a << j.numeroMapa << '\n' << j.n << '\n' << j.titulo << '\n';
    a << j.orientacao << ',' << j.movimentos << ',' << j.rotacoes << ','
      << j.jogador.lin << ',' << j.jogador.col << ',' << j.sobJogador << '\n';
    for (int i=0; i<j.n; i++) {
        for (int c=0; c<j.n; c++) { if (c) a << ','; a << j.cenario[i][c]; }
        a << '\n';
    }
    return static_cast<bool>(a);
}
bool estadoSalvoValido(const Jogo &x) {
    if (x.numeroMapa <= 0 || x.n < 11 || x.n > TAM || x.titulo.empty() ||
        x.titulo.find(',') != string::npos || x.orientacao < 0 || x.orientacao > 3 ||
        x.movimentos < 0 || x.rotacoes < 0 || x.jogador.lin < 0 || x.jogador.lin >= x.n ||
        x.jogador.col < 0 || x.jogador.col >= x.n ||
        (x.sobJogador != VAZIO && x.sobJogador != ALAVANCA && x.sobJogador != SAIDA)) return false;
    int jogadores = 0;
    for (int i=0; i<x.n; i++) for (int c=0; c<x.n; c++) {
        int cel = x.cenario[i][c];
        if (cel < VAZIO || cel > PORTA_B) return false;
        if (cel == JOGADOR) jogadores++;
    }
    return jogadores == 1 && x.cenario[x.jogador.lin][x.jogador.col] == JOGADOR;
}
bool carregaSalvo(Jogo &j) {
    ifstream a("jogo_salvo.csv"); if (!a) return false;
    string l; int mapa=0, n=0;
    if (!getline(a,l)) return false;
    try { size_t p=0; mapa=stoi(l,&p); if (p!=l.size()) return false; } catch (...) { return false; }
    if (!getline(a,l)) return false;
    try { size_t p=0; n=stoi(l,&p); if (p!=l.size()) return false; } catch (...) { return false; }
    if (mapa <= 0 || n < 11 || n > TAM) return false;
    string titulo; if (!getline(a,titulo) || titulo.empty() || titulo.find(',') != string::npos) return false;
    string meta; if (!getline(a,meta)) return false;
    string p; int v[6]{}; int q=0; stringstream ss(meta);
    while (getline(ss,p,',')) {
        if (q >= 6 || p.empty()) return false;
        try { size_t fim=0; v[q]=stoi(p,&fim); if (fim!=p.size()) return false; }
        catch (...) { return false; }
        q++;
    }
    if (q != 6) return false;
    Jogo x; x.numeroMapa=mapa; x.n=n; x.titulo=titulo; x.orientacao=v[0];
    x.movimentos=v[1]; x.rotacoes=v[2]; x.jogador={v[3],v[4]}; x.sobJogador=v[5]; limpaCenario(x);
    for (int i=0; i<n; i++) {
        if (!getline(a,l)) return false;
        int vals[TAM]{}; if (!leLinhaCSV(l,n,vals)) return false;
        for (int c=0; c<n; c++) x.cenario[i][c]=vals[c];
    }
    if (getline(a,l)) return false;
    if (!estadoSalvoValido(x)) return false;
    j=x; return true;
}
void invalidaSalvo(){ remove("jogo_salvo.csv"); }

bool leRegistro(const string &linha, Recorde &r) {
    string campos[4], campo; stringstream ss(linha); int q=0;
    while (getline(ss,campo,',')) { if (q>=4) return false; campos[q++]=campo; }
    if (q!=4 || campos[0].empty() || campos[0].size()>15) return false;
    try {
        size_t p=0; r.nome=campos[0]; r.mapa=stoi(campos[1],&p); if(p!=campos[1].size())return false;
        r.movimentos=stoi(campos[2],&p); if(p!=campos[2].size())return false;
        r.rotacoes=stoi(campos[3],&p); if(p!=campos[3].size())return false;
    } catch (...) { return false; }
    return r.mapa>0 && r.movimentos>=0 && r.rotacoes>=0;
}
int carregaRecordes(Recorde registros[], string &erro) {
    ifstream a("recordes.csv"); if (!a) { erro.clear(); return 0; }
    string linha; int n=0;
    while (getline(a,linha)) {
        if (linha.empty()) continue;
        if (n>=MAX_RECORDES) { erro="recordes.csv possui mais de 200 registros"; return -1; }
        if (!leRegistro(linha,registros[n])) continue;
        n++;
    }
    erro.clear(); return n;
}
void ordenaRecordes(Recorde registros[], int n, int mapa) {
    for (int i=0; i<n; i++) for (int k=i+1; k<n; k++)
        if (registros[k].mapa==mapa &&
            (registros[i].mapa!=mapa || registros[k].movimentos<registros[i].movimentos ||
             (registros[k].movimentos==registros[i].movimentos && registros[k].rotacoes<registros[i].rotacoes)))
            trocar(registros[i],registros[k]);
}
bool gravaRecorde(const Recorde &r, string &erro) {
    if (r.nome.empty() || r.nome.size()>15 || r.nome.find(',')!=string::npos || r.mapa<=0 ||
        r.movimentos<0 || r.rotacoes<0) { erro="dados de recorde invalidos"; return false; }
    int linhas=0; string linha; ifstream existente("recordes.csv");
    while (getline(existente,linha)) if (!linha.empty()) linhas++;
    if (linhas>=MAX_RECORDES) { erro="limite de 200 recordes atingido"; return false; }
    ofstream a("recordes.csv",ios::app); if (!a) { erro="nao foi possivel abrir recordes.csv"; return false; }
    a<<r.nome<<','<<r.mapa<<','<<r.movimentos<<','<<r.rotacoes<<'\n';
    erro = a ? "" : "falha ao gravar recorde"; return static_cast<bool>(a);
}
bool registraRecorde(const Jogo &j) {
    Recorde r; cout<<"Nome (ate 15 caracteres, sem virgula): "; cin>>r.nome;
    r.mapa=j.numeroMapa; r.movimentos=j.movimentos; r.rotacoes=j.rotacoes;
    string erro; if (!gravaRecorde(r,erro)) { cout<<erro<<".\n"; return false; } return true;
}
void mostraRecordes() {
    int mapa; cout<<"Mapa: "; cin>>mapa; Recorde registros[MAX_RECORDES]; string erro;
    int n=carregaRecordes(registros,erro); if(n<0){cout<<erro<<".\n";return;}
    ordenaRecordes(registros,n,mapa); cout<<"\nRecordes do mapa "<<mapa<<"\n"; int exib=0;
    for(int i=0;i<n&&exib<5;i++) if(registros[i].mapa==mapa){cout<<exib+1<<". "<<registros[i].nome<<" - movimentos: "<<registros[i].movimentos<<", rotacoes: "<<registros[i].rotacoes<<'\n';exib++;}
    if(!exib) cout<<"Nenhum recorde registrado.\n";
    cout<<"Pressione ENTER.";cin.ignore();cin.get();
}

bool leSolucao(int mapa, string &sol, string &erro) {
    ifstream a("mapa"+to_string(mapa)+".sol");
    if (!a) { erro="arquivo .sol inexistente"; return false; }
    if (!getline(a,sol) || sol.empty()) { erro="arquivo .sol vazio"; return false; }
    for (size_t i=0; i<sol.size(); i++) {
        if (!strchr("WwAaSsDdQqEe",sol[i])) {
            erro="tecla invalida na posicao "+to_string(i+1);
            return false;
        }
    }
    string extra; if (getline(a,extra) && !extra.empty()) { erro="arquivo .sol deve conter uma unica linha"; return false; }
    return true;
}
string executaSolucao(Jogo inicial,const string &sol,bool desenharPassos) {for(size_t i=0;i<sol.size();i++){string msg;bool ok=processaTecla(inicial,sol[i],msg);if(desenharPassos){desenha(inicial);cout<<"Tecla "<<i+1<<"/"<<sol.size()<<": "<<sol[i]<<"\n";}if(!ok)return "derrota na tecla "+to_string(i+1);if(!msg.empty()&&msg!="Tecla invalida.")return msg;if(venceu(inicial))return "solucao valida ("+to_string(i+1)+" teclas, "+to_string(inicial.movimentos)+" movimentos, "+to_string(inicial.rotacoes)+" rotacoes)";}return venceu(inicial)?"solucao valida":"teclas acabaram sem chegar a saida";}
void verSolucao() {
    int mapa; cout << "Mapa (0 para verificar todos): "; cin >> mapa;
    if (mapa==0) {
        for (int i=1; i<=quantidadeMapas(); i++) {
            Jogo j; string erro, sol;
            if (!carregaMapa(i,j,erro)) { cout<<i<<": invalido - "<<erro<<'\n'; continue; }
            if (!leSolucao(i,sol,erro)) cout<<i<<": "<<erro<<'\n';
            else cout<<i<<" - "<<j.titulo<<": "<<executaSolucao(j,sol,false)<<'\n';
        }
        cout<<"Pressione ENTER."; cin.ignore(); cin.get(); return;
    }
    Jogo j; string erro, sol;
    if (!carregaMapa(mapa,j,erro)) { cout<<"Mapa indisponivel: "<<erro<<'\n'; return; }
    if (!leSolucao(mapa,sol,erro)) { cout<<erro<<".\n"; return; }
    cout<<executaSolucao(j,sol,true)<<"\nPressione ENTER."; cin.ignore(); cin.get();
}

void joga(Jogo &j){bool ativo=true;while(ativo){desenha(j);char t=lerTecla();if(t==27){salvaJogo(j);return;}if(t=='r'||t=='R'){Jogo novo;string e;if(carregaMapa(j.numeroMapa,novo,e))j=novo;continue;}string m;bool ok=processaTecla(j,t,m);if(!ok||venceu(j)){desenha(j);cout<<'\n'<<(ok?"Parabens, voce venceu! ":"")<<m<<'\n';if(venceu(j)){invalidaSalvo();registraRecorde(j);}cout<<"Pressione ENTER.";cin.ignore();cin.get();return;}}
}
int escolheMapaAleatorio(int total) {
    int quantidadeValidos=0, escolhido=0;
    for (int i=1; i<=total; i++) {
        Jogo j; string erro;
        if (carregaMapa(i,j,erro)) {
            quantidadeValidos++;
            if (rand()%quantidadeValidos==0) escolhido=i;
        }
    }
    return escolhido;
}
void novoJogo(){
    int total=quantidadeMapas(); if(total==0){cout<<"Nenhum mapa encontrado.\n";return;}
    cout<<"Mapas disponiveis:\n";
    for(int i=1;i<=total;i++){Jogo j;string e;if(carregaMapa(i,j,e))cout<<i<<" - "<<j.titulo<<"\n";else cout<<i<<" - indisponivel ("<<e<<")\n";}
    cout<<"0 - mapa aleatorio\nEscolha o mapa: "; int escolha;
    if(!(cin>>escolha)){cin.clear();cin.ignore(10000,'\n');cout<<"Entrada invalida; retornando ao menu.\n";return;}
    int numero=(escolha==0)?escolheMapaAleatorio(total):escolha;
    if(escolha<0||escolha>total||numero==0){cout<<"Mapa invalido ou nenhum mapa valido.\n";return;}
    Jogo j; string erro; if(!carregaMapa(numero,j,erro)){cout<<"Mapa indisponivel: "<<erro<<"\n";return;}
    if(ifstream("jogo_salvo.csv")){char c;cout<<"Existe jogo salvo. Substituir? (S/N) ";cin>>c;if(toupper(c)!='S')return;invalidaSalvo();}joga(j);
}
void sobre(){cout<<"\nLabirinto Giratorio - Algoritmos e Programacao II\nM2 - Labirinto Giratorio: A Evolucao\nEquipe: Artur Ximendes, Bruno Delfino e Pedro de Moraes\nProfessor: Thiago Felski pereira/projeto entregue\nDesenvolvimento: 17/Outubro/2026 (PDF datado de 08/10/2026)\n\nObjetivo: chegar a saida usando movimentos, alavancas, rotacoes, portas e gravidade.\nWASD move; Q/E giram sobre alavanca; R reinicia; ESC salva e volta ao menu.\nVer solucao reproduz os arquivos .sol; Recordes mostra os cinco melhores.\nPressione ENTER.";cin.ignore();cin.get();}
void continuarJogo(){Jogo j;if(carregaSalvo(j))joga(j);else cout<<"Nenhum jogo salvo valido.\n";}
int exibeMenu(){
    while(true){
        limparTela();cout<<"\nLABIRINTO GIRATORIO\n1 - Novo jogo\n2 - Continuar\n3 - Ver solucao\n4 - Recordes\n5 - Sobre\n6 - Fim\nOpcao: ";
        int op;if(cin>>op&&op>=1&&op<=6)return op;
        cin.clear();cin.ignore(10000,'\n');cout<<"Opcao invalida. Escolha uma opcao de 1 a 6.\n";
    }
}
int main(){srand(static_cast<unsigned>(time(nullptr)));bool executando=true;while(executando){int op=exibeMenu();if(op==6){executando=false;continue;}switch(op){case 1:novoJogo();break;case 2:continuarJogo();break;case 3:verSolucao();break;case 4:mostraRecordes();break;case 5:sobre();break;}}return 0;}
