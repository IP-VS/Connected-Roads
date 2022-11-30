class MeshNode {
    id: number;
    name: string;
    status: string;
    constructor(id: number, name: string, status: string) {
        this.id = id;
        this.name = name;
        this.status = status;
     }
    toString() {
        return JSON.stringify(this);
    }
    fromJson(json: string) {
        var obj = JSON.parse(json);
        this.id = obj.id;
        this.name = obj.name;
        this.status = obj.status;
    }
}

class NodeList {
    static nodes: MeshNode[] = [];
    static addNode(node: MeshNode) {
        this.nodes.push(node);
    }
    static removeNode(id: number) {
        var index = this.nodes.findIndex(n => n.id == id);
        if (index > -1) {
            this.nodes.splice(index, 1);
            return true;
        }
        return false;
    }
    static getNode(id: number) {
        return this.nodes.find(n => n.id == id);
    }
    static toString() {
        return JSON.stringify(this.nodes);
    }
    static fromJson(json: string) {
        var obj = JSON.parse(json);
        this.nodes = obj.nodes;
    }
}

export {MeshNode, NodeList};